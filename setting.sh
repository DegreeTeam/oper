sudo service hostapd restart
sudo bash /home/pi/oper/ble.sh
sudo echo nameserver 8.8.8.8 nameserver 8.8.4.4 > /etc/resolv.conf
sudo modprobe ipt_MASQUERADE
sudo iptables -t nat -A POSTROUTING -o eth0 -j MASQUERADE
sudo /home/pi/HamProject/jangtest3
