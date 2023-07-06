#ifndef SCSI_DEV_H
#define SCSI_DEV_H

/*
 * This is just a skeleton code to access data on
 * a mass storage device via SCSI protocol
 */

enum scsi_status_e { SCSI_OFF = 0, SCSI_ON };

typedef enum scsi_status_e scsi_status;

struct scsi_dev {
	int block_size;
	int total_block_count;

	int op_status;
	int lock;
	void *driver;
	int (*read)(void *driver, int cdb_size, char *cdb_data, int len,
		    char *data);
	int (*write)(void *driver, int cdb_size, char *cdb_data, int len,
		     char *data);
};

/*
 * SCSI devices must implement this interface and
 * present it to this driver
 */
struct scsi_ifc {
	void *driver;
	int (*read)(void *driver, int cdb_size, char *cdb_data, int len,
		    char *data);
	int (*write)(void *driver, int cdb_size, char *cdb_data, int len,
		     char *data);
};

void scsi_static_init(void);
int scsi_init(struct scsi_ifc *ifc);
int scsi_read(int block_start, int block_count, char *data);
int scsi_write(int block_start, int block_count, char *data);
void scsi_free();
int scsi_up();


/* Command description block 6 byte long structure */
struct command_descriptor_block6 {
	uint8_t op_code;
	uint8_t block_address[3];
	uint8_t length;
	uint8_t control;
} __attribute__((packed));

/* Command description block 10 byte long structure */
struct command_descriptor_block10 {
	uint8_t op_code;
	uint8_t misc_CBD_and_service;
	uint32_t logical_block_address;
	uint8_t misc_CBD;
	uint16_t length;
	uint8_t control;
} __attribute__((packed));

/* Operation codes */
#define CDB_TEST_UNIT_READY 0x00
#define CDB_REQUEST_SENSE 0x03
#define CDB_READ_CAPACITY10 0x25
#define CDB_READ6 0x08
#define CDB_READ10 0x28
#define CDB_WRITE6 0x0A
#define CDB_WRITE10 0x2A

#define SCSI_RC_GOOD 0
#define SCSI_RC_FAILED 1

#define SCSI_READ 0
#define SCSI_WRITE 1

#endif
