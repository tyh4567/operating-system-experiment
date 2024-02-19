#!/bin/bash
Linux_new_id=`ps -ef | grep webserver | grep -v "grep" | awk '{print $2}'`
sem_new_id=`ipcs -s| grep -v "grep" | grep -n '[0-9]' | awk '{print $2}'`
shm_new_id=`ipcs -m| grep -v "grep" | grep -n '[0-9]' | awk '{print $2}'`
echo $Linux_new_id
echo $shm_new_id
echo $sem_new_id
for id in $Linux_new_id
do
    kill -9 $id  
    echo "killed $id"  
done

for id in $sem_new_id
do
    ipcrm -s $id
    echo "ipcrm -s $id"  
done

for id in $shm_new_id
do
    ipcrm -m $id
    echo "ipcrm -m $id"  
done
	rm -rf webserver
	rm -rf *.o
	rm -rf /home/tyh/syse/sysecachetp/web/nweb.log