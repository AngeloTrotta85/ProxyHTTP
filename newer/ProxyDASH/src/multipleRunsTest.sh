#!/bin/bash
mkdir logs

PORT=0   #initialize
while [ "$PORT" -le 10000 ]
do
  PORT=$RANDOM
done


#PORT=20000
COUNTER=1
for runs in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26
do
	sudo ./tc.sh clean eth0
	sudo ./tc.sh clean wlan0

	echo "Clear iptables"
	sudo iptables -t nat -F
	sleep 2

	./script.py
	chmod +x trace.sh
	# SET
	DIR="logs/caba"
	mkdir ${DIR}

	LOGNAME="logs/caba/test_${runs}"
	rm -rf /home/atrotta/.config/google-chrome/Default
	rm -rf /home/atrotta/.cache/google-chrome/
	google-chrome http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sleep 15
	PIDC=`ps aux|grep chrome|grep mediapm|awk '{print $2}'`
	kill -9 ${PIDC}

	sleep 2
	
	echo "Now launching on port: ${PORT}"
	./ProxyDASH -p $PORT -l $LOGNAME -i lo -d -t 5 &
	sudo iptables -t nat -A OUTPUT -p tcp --sport $PORT -j ACCEPT
	sudo iptables -t nat -A OUTPUT -p tcp --sport 10000:65535 -d 143.205.176.132 -j DNAT --to 127.0.0.1:$PORT
	sleep 3
	google-chrome http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sudo ./trace.sh &
	PORT=`echo $PORT + 1 | bc`
	# KILL
	PIDC=`ps aux|grep chrome|grep mediapm|awk '{print $2}'`
	PIDP=`ps aux|grep Proxy|grep DASH|awk '{print $2}'`
	PIDT=`ps aux|grep "sh ./trace.sh"|awk '{print $2}'`
	sleep 300
	kill -9 $PIDC && kill -9 $PIDP && sudo kill -9 $PIDT
done
sudo ./tc.sh clean eth0
sudo ./tc.sh clean wlan0
