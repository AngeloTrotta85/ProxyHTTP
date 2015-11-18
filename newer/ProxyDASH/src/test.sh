eth0=$2
wlan0=$3
echo ${eth0}
echo ${wlan0}

# create additional routing table
#sudo echo 11 dev1table >> /etc/iproute2/rt_tables
#sudo echo 12 dev2table >> /etc/iproute2/rt_tables
if [ "$1" = "clean" ]
then
	sudo ip route del 192.168.1.0/24 dev eth0 src ${eth0} table dev1table
	sudo ip route del default via 192.168.1.254 table dev1table

	# 10.200.0.0/16 is the IP_network for wlan0 - 10.200.255.254 is the gateway
	sudo ip route del 10.0.0.0/16 dev wlan0 src ${wlan0} table dev2table
	sudo ip route del default via 10.0.0.1 table dev2table

	# set up the routing tables
	sudo ip rule del from ${eth0} table dev1table
	sudo ip rule del from ${wlan0} table dev2table

	sudo ip route flush cache
	exit 0
fi
# 130.136.37.0/24 is the IP_network for eth0 - 130.136.37.1 is the gateway
sudo ip route add 192.168.1.0/24 dev eth0 src ${eth0} table dev1table
sudo ip route add default via 192.168.1.254 table dev1table

# 10.200.0.0/16 is the IP_network for wlan0 - 10.200.255.254 is the gateway
sudo ip route add 10.0.0.0/16 dev wlan0 src ${wlan0} table dev2table
sudo ip route add default via 10.0.0.1 table dev2table

# set up the routing tables
sudo ip rule add from ${eth0} table dev1table
sudo ip rule add from ${wlan0} table dev2table

sudo ip route flush cache
