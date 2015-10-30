#!/bin/bash
mkdir logs

PORT=0   #initialize
while [ "$PORT" -le 10000 ]
do
  PORT=$RANDOM
done

COMMAND="chromium-browser"
DIRECTORY="chromium"

#PORT=20000
COUNTER=1
for runs in 1 2 3 4 5 6 7 8 9 10 11 12
do
	sudo ./tc.sh clean eth0
	sudo ./tc.sh clean wlan0
	./script.py

	echo "Clear iptables"
	sudo iptables -t nat -F
	sleep 2

	chmod +x trace.sh
	# SET
	DIR="logs/tvws"
	mkdir ${DIR}
	DIR="logs/wifi"
	mkdir ${DIR}
	DIR="logs/caba_p_random"
	mkdir ${DIR}
	DIR="logs/caba_p_fixed"
	mkdir ${DIR}
	DIR="logs/caba"
	mkdir ${DIR}

	LOGNAME="logs/tvws/test_${runs}"
	rm -rf /home/$USER/.config/$DIRECTORY/Default
	rm -rf /home/$USER/.cache/$DIRECTORY/

	echo "Now launching on port: ${PORT}"
	./ProxyDASH -p $PORT -l $LOGNAME -i lo,lxcbr0,wlan0 -t 1 &
	sleep 3
	$COMMAND --proxy-server="localhost:${PORT}" "http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd&autoplay=true" &
	PORT=`echo $PORT + 1 | bc`
	# KILL
	PIDC=`ps aux|grep $COMMAND|grep mediapm|awk '{print $2}'`
	PIDP=`ps aux|grep Proxy|grep DASH|awk '{print $2}'`
	PIDT=`ps aux|grep trace.sh|awk '{print $2}'`
	sleep 300
	kill -9 $PIDC && kill -9 $PIDP && sudo kill -9 $PIDT

	sudo ./tc.sh clean eth0
	sudo ./tc.sh clean wlan0

	sleep 2

	LOGNAME="logs/wifi/test_${runs}"
	rm -rf /home/$USER/.config/$DIRECTORY/Default
	rm -rf /home/$USER/.cache/$DIRECTORY/
	$COMMAND http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sleep 15
	PIDC=`ps aux|grep $COMMAND|grep mediapm|awk '{print $2}'`
	kill -9 ${PIDC}

	sleep 2
	
	echo "Now launching on port: ${PORT}"
	./ProxyDASH -p $PORT -l $LOGNAME -i lo,lxcbr0,wlan0 -t 1 &
	sleep 3
	$COMMAND http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sudo ./trace.sh &
	PORT=`echo $PORT + 1 | bc`
	# KILL
	PIDC=`ps aux|grep $COMMAND|grep mediapm|awk '{print $2}'`
	PIDP=`ps aux|grep Proxy|grep DASH|awk '{print $2}'`
	PIDT=`ps aux|grep trace.sh|awk '{print $2}'`
	sleep 300
	kill -9 $PIDC && kill -9 $PIDP && sudo kill -9 $PIDT

	sudo ./tc.sh clean eth0
	sudo ./tc.sh clean wlan0

	sleep 2

	LOGNAME="logs/caba_p_random/test_${runs}"
	rm -rf /home/$USER/.config/$DIRECTORY/Default
	rm -rf /home/$USER/.cache/$DIRECTORY/
	$COMMAND http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sleep 15
	PIDC=`ps aux|grep $COMMAND|grep mediapm|awk '{print $2}'`
	kill -9 ${PIDC}

	sleep 2
	
	echo "Now launching on port: ${PORT}"
	./ProxyDASH -p $PORT -l $LOGNAME -i lo -r -a r -t 1 &
	sleep 3
	$COMMAND http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sudo ./trace.sh &
	PORT=`echo $PORT + 1 | bc`
	# KILL
	PIDC=`ps aux|grep $COMMAND|grep mediapm|awk '{print $2}'`
	PIDP=`ps aux|grep Proxy|grep DASH|awk '{print $2}'`
	PIDT=`ps aux|grep trace.sh|awk '{print $2}'`
	sleep 300
	kill -9 $PIDC && kill -9 $PIDP && sudo kill -9 $PIDT

	sudo ./tc.sh clean eth0
	sudo ./tc.sh clean wlan0

	sleep 2

	LOGNAME="logs/caba_p_fixed/test_${runs}"
	rm -rf /home/$USER/.config/$DIRECTORY/Default
	rm -rf /home/$USER/.cache/$DIRECTORY/
	$COMMAND http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sleep 15
	PIDC=`ps aux|grep $COMMAND|grep mediapm|awk '{print $2}'`
	kill -9 ${PIDC}

	sleep 2
	
	echo "Now launching on port: ${PORT}"
	./ProxyDASH -p $PORT -l $LOGNAME -i lo -a f -t 1 &
	sleep 3
	$COMMAND http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sudo ./trace.sh &
	PORT=`echo $PORT + 1 | bc`
	# KILL
	PIDC=`ps aux|grep $COMMAND|grep mediapm|awk '{print $2}'`
	PIDP=`ps aux|grep Proxy|grep DASH|awk '{print $2}'`
	PIDT=`ps aux|grep trace.sh|awk '{print $2}'`
	sleep 300
	kill -9 $PIDC && kill -9 $PIDP && sudo kill -9 $PIDT

	sudo ./tc.sh clean eth0
	sudo ./tc.sh clean wlan0

	sleep 2

	LOGNAME="logs/caba/test_${runs}"
	rm -rf /home/$USER/.config/$DIRECTORY/Default
	rm -rf /home/$USER/.cache/$DIRECTORY/
	$COMMAND http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sleep 1 #TODO Was sleep 15
	PIDC=`ps aux|grep $COMMAND|grep mediapm|awk '{print $2}'`
	kill -9 ${PIDC}

	sleep 2
	
	echo "Now launching on port: ${PORT}"
	./ProxyDASH -p $PORT -l $LOGNAME -i lo -t 1 &
	sleep 3
	$COMMAND http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sudo ./trace.sh &
	PORT=`echo $PORT + 1 | bc`
	# KILL
	PIDC=`ps aux|grep $COMMAND|grep mediapm|awk '{print $2}'`
	PIDP=`ps aux|grep Proxy|grep DASH|awk '{print $2}'`
	PIDT=`ps aux|grep trace.sh|awk '{print $2}'`
	sleep 1 #TODO Was sleep 300
	kill -9 $PIDC && kill -9 $PIDP && sudo kill -9 $PIDT

	sudo ./tc.sh clean eth0
	sudo ./tc.sh clean wlan0
done
sudo ./tc.sh clean eth0
sudo ./tc.sh clean wlan0
