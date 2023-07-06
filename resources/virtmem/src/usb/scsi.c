#include "../util.h"
#include "../thread.h"
#include "scsi.h"
#include "allocator.h"
#include "debug.h"
#include "error.h"

DEBUG_NAME("SCSI");

/* 
 * For purpose of this OS we assume there is only 
 * one SCSI device which is a USB flash drive 
 */
static struct scsi_dev *scsi = NULL;
static int scsi_dev_lock;

static int scsi_read_write(int dir, int block_start,
    int block_count, char *data);

static int scsi_read_capacity(void);
static int scsi_test_unit_ready(void);

void scsi_static_init(void) {
  spinlock_init(&scsi_dev_lock);
}

int scsi_init(struct scsi_ifc *ifc) {
  int attempts;
  int rc;

  spinlock_acquire(&scsi_dev_lock);
  if (scsi != NULL) {
    /* A second USB disk has been inserted -> reject it */
    spinlock_release(&scsi_dev_lock);
    return ERR_PROTO;
  }

  scsi = kzalloc(sizeof(struct scsi_dev));
  if (scsi == NULL) {
    spinlock_release(&scsi_dev_lock);
    return ERR_NO_MEM;
  }

  /* Copy the driver interface */
  scsi->driver = ifc->driver;
  scsi->read = ifc->read;
  scsi->write = ifc->write;

  rc = scsi_test_unit_ready();

  attempts = 20;

  while ((rc < 0) && (attempts-- > 0)) {
    ms_delay(500);
    rc = scsi_test_unit_ready();
  };

  if (rc < 0) {
    scsi_free(scsi);
    spinlock_release(&scsi_dev_lock);
    return ERR_PROTO;
  }

  rc = scsi_read_capacity();
  DEBUG("Reading device capacity parameters %s", DEBUG_STATUS(rc));

  if(rc < 0) {
    spinlock_release(&scsi_dev_lock);
    scsi_free(scsi);
    return ERR_PROTO;
  }

  DEBUG("Device block size %d, block count %d", 
      scsi->block_size, scsi->total_block_count);

  spinlock_release(&scsi_dev_lock);

  return 0;
}

/* Frees the SCSI device, releasing resources */
void scsi_free() {
  /* If any operation is in progress wait to finish */
  spinlock_acquire(&scsi_dev_lock);

  /* We can safely release resources and clear the pointer */
  if (scsi != NULL) 
    kfree(scsi);

  scsi = NULL;

  spinlock_release(&scsi_dev_lock);
}

int scsi_up() {
  return ((scsi == NULL) ? 0 : 1);
}

struct sense_data {
  uint8_t error_code: 7;
  uint8_t valid: 1;
  uint8_t reserved0;
  uint8_t sense_key: 4;
  uint8_t reserved1: 4;
  uint8_t information[4];
  uint8_t additional_sense_length;
  uint8_t reserved2[4];
  uint8_t additional_sense_code;
  uint8_t additional_sense_code_qualifier;
  uint8_t reserved3[4];
}__attribute__((packed));

static void scsi_request_sense(void) {
  struct command_descriptor_block10 cdb10;
  struct sense_data sdata;
  int rc;

  bzero((char *)&sdata, sizeof(sdata));

  bzero((char *)&cdb10, sizeof(cdb10));
  cdb10.op_code = CDB_REQUEST_SENSE;
  cdb10.length = sizeof(struct sense_data);

  rc = scsi->read(scsi->driver, sizeof(cdb10), (char *)&cdb10, sizeof(sdata), (char *)&sdata);
  DEBUG("scsi_request_sense() :: rc was %d", rc);

  DEBUG("Is valid? %d", sdata.valid);
  DEBUG("Error code: %x", sdata.error_code);
  DEBUG("Sense key: %x", sdata.sense_key);
}

static int scsi_test_unit_ready(void) {
  struct command_descriptor_block6 cdb6;
  int rc;

  bzero((char *)&cdb6, sizeof(cdb6));
  cdb6.op_code = CDB_TEST_UNIT_READY;

  rc = scsi->read(scsi->driver, sizeof(cdb6), (char *)&cdb6, 0, NULL);

  if (rc == SCSI_RC_FAILED) {
    DEBUG("Device is not ready");

    /* Send REQUEST SENSE command. It should contain info about what went wrong in previous command,
     * and, more importantly, if we have a UNIT ATTENTION condition (e.g. media change or power on reset),
     * this condition will be cleared by the device (meaning the next retry might succeed).
     */
    scsi_request_sense();
    return -1;
  }

  return 0;
}
/* 
 * Read driver parameters 
 */
struct capacity_data {
  uint32_t total_block_count;
  uint32_t block_size;
} __attribute__((packed));

static int scsi_read_capacity(void) {
  struct command_descriptor_block10 cdb10;
  struct capacity_data cap_data;
  int cap_data_size = sizeof(struct capacity_data);
  int rc;

  bzero((char *)&cdb10, sizeof(cdb10));
  cdb10.op_code = CDB_READ_CAPACITY10;

  rc = scsi->read(scsi->driver, sizeof(cdb10), (char *)&cdb10, 
      cap_data_size, (char *)&cap_data);

  if (rc != SCSI_RC_GOOD) 
    return -1;

  scsi->total_block_count = ntohl(cap_data.total_block_count);
  scsi->block_size = ntohl(cap_data.block_size);

  return 0;
}

/*
 * Read a number of blocks from the drive
 */
static int scsi_read_write(int dir, int block_start, 
    int block_count, char *data) {
  int transfer_len;
  int rc;

  spinlock_acquire(&scsi_dev_lock);

  if (scsi == NULL) {
    spinlock_release(&scsi_dev_lock);
    return -1;
  }


  /* Command to SCSI server on the USB mass storage device */
  struct command_descriptor_block10 cdb10 = {
    .op_code = (dir == SCSI_READ) ? CDB_READ10: CDB_WRITE10,
    .misc_CBD_and_service = 0,
    .logical_block_address = htonl(block_start),
    .misc_CBD = 0,
    .length = htons(block_count),  /* Unit of sectors */
    .control = 0
  };

  transfer_len = block_count * scsi->block_size;

  if (dir == SCSI_READ)
    rc = scsi->read(scsi->driver, sizeof(cdb10), (char *)&cdb10, 
        transfer_len, data);
  else
    rc = scsi->write(scsi->driver, sizeof(cdb10), (char *)&cdb10, 
        transfer_len, data);

  spinlock_release(&scsi_dev_lock);

  if (rc != SCSI_RC_GOOD) {
    return -1;
  }

  return 0;
}
/*
 * SCSI interface functions 
 */
int scsi_read(int block_start, int block_count, char *data) {
  return scsi_read_write(SCSI_READ, block_start, block_count, data);
}

int scsi_write(int block_start, int block_count, char *data) {
  return scsi_read_write(SCSI_WRITE, block_start, block_count, data);
}

