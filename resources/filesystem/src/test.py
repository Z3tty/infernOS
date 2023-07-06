import os, sys, string, time, subprocess

fsck_implemented = False
flush_implemented = False

def do_fsck() :
    if (fsck_implemented==True) :
        p.stdin.write('fsck\n')
        p.stdin.write('ls\n')
    else :
        p.stdin.write('ls\n')
    p.stdin.flush()

def check(input, output) :
    p.stdin.write(input)
    p.stdin.flush()
    p.stdout.flush()
    out = p.stdout.read()
    print out
    if out != output :
        print "File content does not match, command", input
    else:
        print "ok\n"
        
def do_exit():
    p.stdin.write('exit\n')
    return p.communicate()[0]
    
def test1():
    print "----------Starting Test1----------\n"
    p.stdin.write('mkfs\n')
    do_fsck()
    p.stdin.write('cat foo\n')
    p.stdin.write('.\n')
    p.stdin.write('stat foo\n')
    p.stdin.write('mkdir d1\n')
    p.stdin.write('stat d1\n')
    p.stdin.write('cd d1\n')
    p.stdin.write('ls\n')
    p.stdin.write('cat foo\n')
    p.stdin.write('ABCD\n')
    p.stdin.write('.\n')

    p.stdin.write('ln foo bar\n')
    do_fsck()
    p.stdin.write('ls\n')
    p.stdin.write('cat bar\n')
    p.stdin.write('.\n')
    do_exit()
    print "----------Test1 Finished----------\n"

# test2: open/write/close
def test2() :
    print "----------Starting Test2----------\n"
    do_fsck()
    p.stdin.write('cat testf\n')
    p.stdin.write('WELL\n')
    p.stdin.write('DONE\n')
    p.stdin.write('.\n')
    do_exit()
    print "----------Test2 Finished----------\n"    

#open with readonly test
def test3():
    print "----------Starting Test3----------\n"
    do_fsck()
    p.stdin.write('more testf\n')
    do_exit()
    print "----------Test3 Finished----------\n"    

#test4: rm tests
def test4():
    print "----------Starting Test4----------\n"
    do_fsck()
    p.stdin.write('cd d1\n')
    p.stdin.write('rm foo\n')
    p.stdin.write('ls\n')
    p.stdin.write('more bar\n')  # ABCD
    p.stdin.write('rm bar\n')
    p.stdin.write('ls\n')
    p.stdin.write('more bar\n') # cat should fail
    do_exit()
    print "----------Test4 Finished----------\n"


#test5: simple error cases
def test5() :
    print "----------Starting Test5----------\n"
    do_fsck()
    p.stdin.write('rmdir non_exitsting\n') # Problem with removing directory
    p.stdin.write('cd non_exitsing\n') # Problem with changing directory
    p.stdin.write('ln non_existing non_existing_too\n') # Problem with ln
    p.stdin.write('rm non_existing\n') # Problem with rm
    do_exit()
    print "----------Test5 Finished----------\n"

#test 6: create lots of files in one dir
def test6():
    print "----------Starting Test7----------\n"
    p.stdin.write('mkfs\n')
    p.stdin.write('mkdir d00\n')
    p.stdin.write('cd d00\n')
    for i in range(1, 45):
		p.stdin.write('cat f%d\n' % i)
		p.stdin.write('ABCDE\n')
		p.stdin.write('.\n')
    p.stdin.write('more f1\n')  # ABCDE
    p.stdin.write('stat f3\n')  # 6
    p.stdin.write('more f20\n')   # ABCDE
    do_exit()
    print "----------Test6 Finished----------\n"


#test 7: create a lot of directories
def test7() :
    print "----------Starting Test7----------\n"
    p.stdin.write('mkdir d2\n')
    p.stdin.write('cd d2\n')
    for i in range (1, 50) :
		p.stdin.write('mkdir d%d\n' %i)
		p.stdin.write('stat d%d\n' %i)   #DIRECTORY
		p.stdin.write('cd d%d\n' %i)
    p.stdin.write('cd ..\n')
    p.stdin.write('cd ..\n')
    p.stdin.write('cd ..\n')
    p.stdin.write('stat d47\n') # DIRECTORY
    do_exit()
    print "----------Test7 Finished----------\n"

def spawn_lnxsh():
    global p
    p = subprocess.Popen('./p6sh', shell=True, stdin=subprocess.PIPE)
 
####### Main Test Program #######
print "..........Starting\n\n"
spawn_lnxsh()
test1()
spawn_lnxsh()
test2()
spawn_lnxsh()
test3()
spawn_lnxsh()
test4()
spawn_lnxsh()
test5()
spawn_lnxsh()
test6()
spawn_lnxsh()
test7()
print "\nFinished !"


