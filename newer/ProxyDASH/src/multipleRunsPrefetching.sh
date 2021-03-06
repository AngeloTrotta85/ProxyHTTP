#!/bin/bash
mkdir logs

PORT=0   #initialize
while [ "$PORT" -le 10000 ]
do
  PORT=$RANDOM
done


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
	rm -rf /home/atrotta/.config/google-chrome/Default
	rm -rf /home/atrotta/.cache/google-chrome/
	google-chrome http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sleep 15
	PIDC=`ps aux|grep chrome|grep mediapm|awk '{print $2}'`
	kill -9 ${PIDC}

	sleep 2
	
	echo "Now launching on port: ${PORT}"
	./ProxyDASH -p $PORT -l $LOGNAME -d -i lo,eth0 -t 1 &
	sudo iptables -A OUTPUT -p tcp --sport $PORT -j ACCEPT
	sudo iptables -t nat -A OUTPUT -p tcp --sport 10000:65535 -d 143.205.176.132 -j DNAT --to 127.0.0.1:$PORT
	sleep 3
	google-chrome http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sudo ./trace.sh &
	PORT=`echo $PORT + 1 | bc`
	# KILL
	PIDC=`ps aux|grep chrome|grep mediapm|awk '{print $2}'`
	PIDP=`ps aux|grep Proxy|grep DASH|awk '{print $2}'`
	PIDT=`ps aux|grep trace.sh|awk '{print $2}'`
	sleep 300
	kill -9 $PIDC && kill -9 $PIDP && sudo kill -9 $PIDT

	sudo ./tc.sh clean eth0
	sudo ./tc.sh clean wlan0

	echo "Clear iptables"
	sudo iptables -t nat -F
	sleep 2

	LOGNAME="logs/wifi/test_${runs}"
	rm -rf /home/atrotta/.config/google-chrome/Default
	rm -rf /home/atrotta/.cache/google-chrome/
	google-chrome http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sleep 15
	PIDC=`ps aux|grep chrome|grep mediapm|awk '{print $2}'`
	kill -9 ${PIDC}

	sleep 2
	
	echo "Now launching on port: ${PORT}"
	./ProxyDASH -p $PORT -l $LOGNAME -d -i lo,wlan0 -t 1 &
	sudo iptables -A OUTPUT -p tcp --sport $PORT -j ACCEPT
	sudo iptables -t nat -A OUTPUT -p tcp --sport 10000:65535 -d 143.205.176.132 -j DNAT --to 127.0.0.1:$PORT
	sleep 3
	google-chrome http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sudo ./trace.sh &
	PORT=`echo $PORT + 1 | bc`
	# KILL
	PIDC=`ps aux|grep chrome|grep mediapm|awk '{print $2}'`
	PIDP=`ps aux|grep Proxy|grep DASH|awk '{print $2}'`
	PIDT=`ps aux|grep trace.sh|awk '{print $2}'`
	sleep 300
	kill -9 $PIDC && kill -9 $PIDP && sudo kill -9 $PIDT

	sudo ./tc.sh clean eth0
	sudo ./tc.sh clean wlan0

	echo "Clear iptables"
	sudo iptables -t nat -F
	sleep 2

	LOGNAME="logs/caba_p_random/test_${runs}"
	rm -rf /home/atrotta/.config/google-chrome/Default
	rm -rf /home/atrotta/.cache/google-chrome/
	google-chrome http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sleep 15
	PIDC=`ps aux|grep chrome|grep mediapm|awk '{print $2}'`
	kill -9 ${PIDC}

	sleep 2
	
	echo "Now launching on port: ${PORT}"
	./ProxyDASH -p $PORT -l $LOGNAME -d -i lo -r -a r -t 1 &
	sudo iptables -A OUTPUT -p tcp --sport $PORT -j ACCEPT
	sudo iptables -t nat -A OUTPUT -p tcp --sport 10000:65535 -d 143.205.176.132 -j DNAT --to 127.0.0.1:$PORT
	sleep 3
	google-chrome http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sudo ./trace.sh &
	PORT=`echo $PORT + 1 | bc`
	# KILL
	PIDC=`ps aux|grep chrome|grep mediapm|awk '{print $2}'`
	PIDP=`ps aux|grep Proxy|grep DASH|awk '{print $2}'`
	PIDT=`ps aux|grep trace.sh|awk '{print $2}'`
	sleep 300
	kill -9 $PIDC && kill -9 $PIDP && sudo kill -9 $PIDT

	sudo ./tc.sh clean eth0
	sudo ./tc.sh clean wlan0

	echo "Clear iptables"
	sudo iptables -t nat -F
	sleep 2

	LOGNAME="logs/caba_p_fixed/test_${runs}"
	rm -rf /home/atrotta/.config/google-chrome/Default
	rm -rf /home/atrotta/.cache/google-chrome/
	google-chrome http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sleep 15
	PIDC=`ps aux|grep chrome|grep mediapm|awk '{print $2}'`
	kill -9 ${PIDC}

	sleep 2
	
	echo "Now launching on port: ${PORT}"
	./ProxyDASH -p $PORT -l $LOGNAME -d -i lo -t 1 -a f &
	sudo iptables -A OUTPUT -p tcp --sport $PORT -j ACCEPT
	sudo iptables -t nat -A OUTPUT -p tcp --sport 10000:65535 -d 143.205.176.132 -j DNAT --to 127.0.0.1:$PORT
	sleep 3
	google-chrome http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sudo ./trace.sh &
	PORT=`echo $PORT + 1 | bc`
	# KILL
	PIDC=`ps aux|grep chrome|grep mediapm|awk '{print $2}'`
	PIDP=`ps aux|grep Proxy|grep DASH|awk '{print $2}'`
	PIDT=`ps aux|grep trace.sh|awk '{print $2}'`
	sleep 300
	kill -9 $PIDC && kill -9 $PIDP && sudo kill -9 $PIDT

	sudo ./tc.sh clean eth0
	sudo ./tc.sh clean wlan0

	echo "Clear iptables"
	sudo iptables -t nat -F
	sleep 2

	LOGNAME="logs/caba/test_${runs}"
	rm -rf /home/atrotta/.config/google-chrome/Default
	rm -rf /home/atrotta/.cache/google-chrome/
	google-chrome http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sleep 1 #TODO Was sleep 15
	PIDC=`ps aux|grep chrome|grep mediapm|awk '{print $2}'`
	kill -9 ${PIDC}

	sleep 2
	
	echo "Now launching on port: ${PORT}"
	./ProxyDASH -p $PORT -l $LOGNAME -d -i lo -t 1 &
	sudo iptables -A OUTPUT -p tcp --sport $PORT -j ACCEPT
	sudo iptables -t nat -A OUTPUT -p tcp --sport 10000:65535 -d 143.205.176.132 -j DNAT --to 127.0.0.1:$PORT
	sleep 3
	google-chrome http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
	sudo ./trace.sh &
	PORT=`echo $PORT + 1 | bc`
	# KILL
	PIDC=`ps aux|grep chrome|grep mediapm|awk '{print $2}'`
	PIDP=`ps aux|grep Proxy|grep DASH|awk '{print $2}'`
	PIDT=`ps aux|grep trace.sh|awk '{print $2}'`
	sleep 1 #TODO Was sleep 300
	kill -9 $PIDC && kill -9 $PIDP && sudo kill -9 $PIDT

	sudo ./tc.sh clean eth0
	sudo ./tc.sh clean wlan0
done
sudo ./tc.sh clean eth0
sudo ./tc.sh clean wlan0
