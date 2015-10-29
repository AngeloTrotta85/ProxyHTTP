# Usage
# sudo ./filename.sh {set|clean} interface limit

#network interface on which to limit traffic
IF=$2
#delete existing rules
tc qdisc del dev ${IF} root

iptables -t mangle -F

if [ "$1" = "clean" ]
then
	exit 0
fi

echo "Setting.."


#limit of the network interface in question
LINKCEIL="10gbit"
#limit outbound Bitcoin protocol traffic to this rate
LIMIT=$3

#LIMIT=`echo ${LIMIT} / 5 | bc`
echo $LIMIT


#add root class
tc qdisc add dev ${IF} root handle 1: htb default 10

#add parent class
tc class add dev ${IF} parent 1: classid 1:1 htb rate ${LINKCEIL} ceil ${LINKCEIL}

#add our two classes. one unlimited, another limited
tc class add dev ${IF} parent 1:1 classid 1:10 htb rate ${LINKCEIL} ceil ${LINKCEIL} prio 0
tc class add dev ${IF} parent 1:1 classid 1:11 htb rate ${LIMIT} ceil ${LIMIT} prio 1

#add handles to our classes so packets marked with <x> go into the class with "... handle <x> fw ..."
tc filter add dev ${IF} parent 1: protocol ip prio 1 handle 1 fw classid 1:10
tc filter add dev ${IF} parent 1: protocol ip prio 2 handle 2 fw classid 1:11

#   these packages are filtered by the tc filter with "handle 2"
#   this filter sends the packages into the 1:11 class, and this class is limited to ${LIMIT}
iptables -t mangle -A OUTPUT -p tcp -m tcp -j MARK --set-mark 0x2
