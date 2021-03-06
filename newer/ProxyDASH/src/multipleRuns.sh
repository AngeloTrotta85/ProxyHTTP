#!/bin/bash
mkdir logs

PORT=0   #initialize
while [ "$PORT" -le 10000 ]
do
  PORT=$RANDOM
done


#PORT=20000
COUNTER=1
for runs in 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25
do
for j in 100 16000 29000 42000 55000
do
for i in 55000 42000 29000 16000
do
	# SET
	RATE1=$i
	RATE2=$j
	DIR="logs/${RATE1}_${RATE2}"
	mkdir ${DIR}
	LOGNAME="${DIR}/test_${runs}"
	rm -rf /home/atrotta/.config/google-chrome/Default
	rm -rf /home/atrotta/.cache/google-chrome/
	sudo ./tc.sh clean eth0
	sudo ./tc.sh clean wlan0
	google-chrome http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true &
#google-chrome http://kgit.html5video.org/pulls/442/modules/EmbedPlayerDash/tests/Dash.qunit.html#config={"flashvars":{"dash":{"forceSourceUrl":"http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/2sec/BigBuckBunny_2s_simple_2014_05_09.mpd","includeInLayout":false,"sourceUrl":"http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/2sec/BigBuckBunny_2s_simple_2014_05_09.mpd"}}}
	sleep 15
	PIDC=`ps aux|grep chrome|grep mediapm|awk '{print $2}'`
	kill -9 ${PIDC}

	# LAUNCH
	sudo ./tc.sh set eth0 $RATE1
	sudo ./tc.sh set wlan0 $RATE2

	echo "RATE1 = ${RATE1}, RATE2 = ${RATE2}, LOGNAME = ${LOGNAME}"
	sleep 2
	
	echo "Now launching on port: ${PORT}"
	./ProxyDASH -p $PORT -l $LOGNAME -i lo -d -t 5 &
	sleep 3
#	google-chrome http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/2sec/BigBuckBunny_2s_simple_2014_05_09.mpd\&autoplay=true &
#	google-chrome http://kgit.html5video.org/pulls/442/modules/EmbedPlayerDash/tests/Dash.qunit.html#config={"flashvars":{"dash":{"forceSourceUrl":"http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/2sec/BigBuckBunny_2s_simple_2014_05_09.mpd","includeInLayout":false,"sourceUrl":"http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/2sec/BigBuckBunny_2s_simple_2014_05_09.mpd"}}} --proxy-server="localhost:${PORT}"
	google-chrome http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd\&autoplay=true --proxy-server="localhost:${PORT}" &
	sleep 4
#	export http_proxy="http://localhost:${PORT}"

	PORT=`echo $PORT + 1 | bc`

	# KILL
	PIDC=`ps aux|grep chrome|grep mediapm|awk '{print $2}'`
	PIDP=`ps aux|grep Proxy|grep DASH|awk '{print $2}'`
	sleep 250
	kill -9 $PIDC
	sleep 2
	kill -9 $PIDP
done
done
done
sudo ./tc.sh clean eth0
sudo ./tc.sh clean wlan0
