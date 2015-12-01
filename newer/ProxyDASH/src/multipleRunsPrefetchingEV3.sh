#!/bin/bash
mkdir logs

PORT=0   #initialize
while [ "$PORT" -le 10000 ]
do
  PORT=$RANDOM
done
	
# SET
DIR="logs/tvws"
mkdir ${DIR}
DIR="logs/wifi"
mkdir ${DIR}
DIR="logs/caba_p_random"
mkdir ${DIR}
DIR="logs/caba_p_random/2"
mkdir ${DIR}
DIR="logs/caba_p_random/4"
mkdir ${DIR}
DIR="logs/caba_p_random/6"
mkdir ${DIR}
DIR="logs/caba_p_random/8"
mkdir ${DIR}
DIR="logs/caba_p_random/10"
mkdir ${DIR}
DIR="logs/caba_p_fixed"
mkdir ${DIR}
DIR="logs/caba_p_fixed/2"
mkdir ${DIR}
DIR="logs/caba_p_fixed/4"
mkdir ${DIR}
DIR="logs/caba_p_fixed/6"
mkdir ${DIR}
DIR="logs/caba_p_fixed/8"
mkdir ${DIR}
DIR="logs/caba_p_fixed/10"
mkdir ${DIR}
DIR="logs/caba"
mkdir ${DIR}
DIR="logs/caba/2234145"
mkdir ${DIR}
DIR="logs/caba/1071529"
mkdir ${DIR}
DIR="logs/stepByStep"
mkdir ${DIR}

COMMAND="google-chrome"
DIRECTORY="google-chrome"

#PORT=20000
sudo ./tc.sh clean eth0 90000
sudo ./tc.sh clean wlan0 250000
#sudo ./tc.sh clean wlan0 290000
sudo ./tc.sh set eth0 90000
sudo ./tc.sh set wlan0 250000
#sudo ./tc.sh set wlan0 290000

COUNTER=1
for runs in {1..15}
do
for test in {1..3}
do
	if [ $test -eq 1 ]
	then
		sudo ./tc.sh set eth0 55000
		sudo ./tc.sh set wlan0 290000
	elif [ $test -eq 2 ]
	then
		sudo ./tc.sh set eth0 55000
		sudo ./tc.sh set wlan0 55000
	else
		sudo ./tc.sh set eth0 90000
		sudo ./tc.sh set wlan0 250000
	fi
	sleep 5


	for offset in 2 4 6 8 10
	do

		LOGNAME="logs/caba_p_random/${offset}/test${test}_${runs}"
		echo "$LOGNAME"
		rm -rf /home/$USER/.config/$DIRECTORY/Default
		rm -rf /home/$USER/.cache/$DIRECTORY/

		sleep 2
	
		echo "Now launching on port: ${PORT}"
		./ProxyDASH -p $PORT -l $LOGNAME -i lo -a r -o $offset &
		sleep 3
		$COMMAND --proxy-server="localhost:${PORT}" 'http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd&autoplay=true' &
		PORT=`echo $PORT + 1 | bc`
		# KILL
		PIDC=`ps aux|grep $COMMAND|grep mediapm|awk '{print $2}'`
		PIDP=`ps aux|grep Proxy|grep DASH|awk '{print $2}'`
		PIDT=`ps aux|grep trace.sh|awk '{print $2}'`
		sleep 300
		kill -9 $PIDC && kill -9 $PIDP && sudo kill -9 $PIDT


		sleep 2

		LOGNAME="logs/caba_p_fixed/${offset}/test${test}_${runs}"
		echo "$LOGNAME"
		rm -rf /home/$USER/.config/$DIRECTORY/Default
		rm -rf /home/$USER/.cache/$DIRECTORY/

		sleep 2
	
		echo "Now launching on port: ${PORT}"
		./ProxyDASH -p $PORT -l $LOGNAME -i lo -a f -o $offset &
		sleep 3
		$COMMAND --proxy-server="localhost:${PORT}" 'http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd&autoplay=true' &
		PORT=`echo $PORT + 1 | bc`
		# KILL
		PIDC=`ps aux|grep $COMMAND|grep mediapm|awk '{print $2}'`
		PIDP=`ps aux|grep Proxy|grep DASH|awk '{print $2}'`
		PIDT=`ps aux|grep trace.sh|awk '{print $2}'`
		sleep 300
		kill -9 $PIDC && kill -9 $PIDP && sudo kill -9 $PIDT


		sleep 2
	
	done
	
	sleep 2

	LOGNAME="logs/caba/2234145/test${test}_${runs}"
	echo "$LOGNAME"
	rm -rf /home/$USER/.config/$DIRECTORY/Default
	rm -rf /home/$USER/.cache/$DIRECTORY/

	sleep 2
	
	echo "Now launching on port: ${PORT}"
	./ProxyDASH -p $PORT -l $LOGNAME -i lo -a c -q 2234145 &
	sleep 3
	$COMMAND --proxy-server="localhost:${PORT}" 'http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd&autoplay=true' &
	PORT=`echo $PORT + 1 | bc`
	# KILL
	PIDC=`ps aux|grep $COMMAND|grep mediapm|awk '{print $2}'`
	PIDP=`ps aux|grep Proxy|grep DASH|awk '{print $2}'`
	PIDT=`ps aux|grep trace.sh|awk '{print $2}'`
	sleep 300 #TODO Was sleep 300
	kill -9 $PIDC && kill -9 $PIDP && sudo kill -9 $PIDT
	

	LOGNAME="logs/caba/1071529/test${test}_${runs}"
	echo "$LOGNAME"
	rm -rf /home/$USER/.config/$DIRECTORY/Default
	rm -rf /home/$USER/.cache/$DIRECTORY/

	sleep 2
	
	echo "Now launching on port: ${PORT}"
	./ProxyDASH -p $PORT -l $LOGNAME -i lo -a c -q 1071529 &
	sleep 3
	$COMMAND --proxy-server="localhost:${PORT}" 'http://mediapm.edgesuite.net/dash/public/support-player/current/index.html?source=http://www-itec.uni-klu.ac.at/ftp/datasets/DASHDataset2014/BigBuckBunny/1sec/BigBuckBunny_1s_simple_2014_05_09.mpd&autoplay=true' &
	PORT=`echo $PORT + 1 | bc`
	# KILL
	PIDC=`ps aux|grep $COMMAND|grep mediapm|awk '{print $2}'`
	PIDP=`ps aux|grep Proxy|grep DASH|awk '{print $2}'`
	PIDT=`ps aux|grep trace.sh|awk '{print $2}'`
	sleep 300 #TODO Was sleep 300
	kill -9 $PIDC && kill -9 $PIDP && sudo kill -9 $PIDT

	sleep 2

	
done
done
sudo ./tc.sh clean eth0
sudo ./tc.sh clean wlan0
