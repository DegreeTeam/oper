sudo hciconfig hci0 reset
sudo hciconfig hci0 down
sudo hciconfig hci0 up
sudo hciconfig hci0 name degree
sudo hciconfig hci0 leadv 3
sudo hcitool -i hci0 cmd 0x08 0x0008 1e 02 01 1a 1a ff 4c 00 02 15 31 32 33 34 35 36 37 38 31 32 33 34 35 36 37 38 33 00 00 00 c5 00
