/*
 * fprint.c
 *
 *  Created on: May 17, 2015
 *      Author: juni
 */

#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

int main(void) {
	FILE *fp;
	FILE *fp_in;
	char ssid[9] ={0,};
	char pw[9] ={0,};
	char rssi[2] ={0,};
	char name[1024] ={0,};
	char ip[20] ={0,};
	pid_t pid;
	int i,j;

	char ble1[] = "sudo hcitool -i hci0 cmd 0x08 0x0008 1e 02 01 1a 1a ff 4c 00 02 15";
	if (fp_in = fopen("/home/pi/oper/ssid.config", "r")) {
		fscanf(fp_in, "%s", ssid);
		fclose(fp_in);
	}
	if (fp_in = fopen("/home/pi/oper/pw.config", "r")) {
		fscanf(fp_in, "%s", pw);
		fclose(fp_in);
	}
	if (fp_in = fopen("/home/pi/oper/rssi.config", "r")) {
		fscanf(fp_in, "%s", rssi);
		fclose(fp_in);
	}
	if (fp_in = fopen("/home/pi/oper/name.config", "r")) {
		fscanf(fp_in, "%s", name);
		fclose(fp_in);
	}
	if (fp_in = fopen("/home/pi/oper/ip.config", "r")) {
		fscanf(fp_in, "%s", ip);
		fclose(fp_in);
	}

	//hostapd setting
	if (fp = fopen("/etc/hostapd/hostapd.conf", "w")) {
		fprintf(fp, "interface=wlan0\n");
		fprintf(fp, "driver=rtl871xdrv\n");
		fprintf(fp, "ssid=%s\n", ssid);
		fprintf(fp, "hw_mode=g\n");
		fprintf(fp, "channel=6\n");
		fprintf(fp, "macaddr_acl=0\n");
		fprintf(fp, "auth_algs=1\n");
		fprintf(fp, "ignore_broadcast_ssid=0\n");
		fprintf(fp, "wpa=2\n");
		fprintf(fp, "wpa_passphrase=%s\n", pw);
		fprintf(fp, "wpa_key_mgmt=WPA-PSK\n");
		fprintf(fp, "wpa_pairwise=TKIP\n");
		fprintf(fp, "rsn_pairwise=CCMP\n");
		fclose(fp);
	}else{printf("hostapd error\n");}

	if (fp = fopen("/home/pi/oper/ble.sh", "w")) {
		fprintf(fp, "sudo hciconfig hci0 reset\n");
		fprintf(fp, "sudo hciconfig hci0 down\n");
		fprintf(fp, "sudo hciconfig hci0 up\n");
		fprintf(fp, "sudo hciconfig hci0 name %s\n", name);
		fprintf(fp, "sudo hciconfig hci0 leadv 3\n");

		fprintf(fp, "%s", ble1);
		for(i=0;i<8;i++){
			fprintf(fp, " %02x", ssid[i]);
		}
		for(i=0;i<8;i++){
			fprintf(fp, " %02x", pw[i]);
		}
		fprintf(fp, " %02x 00 00 00 c5 00\n", rssi[0]);
		fclose(fp);
	}else{printf("ble error\n");}
	
	pid = fork();
        switch(pid)
        {
                case -1:
                        printf("fork failed");
                        break;
                case 0:
                        execl("/bin/bash", "bash", "/home/pi/oper/setting.sh", (char*) 0);
                        printf("exec failed");
                        break;
                default:
                        wait((int*) 0);
                        printf("ls completed\n");
                        exit(0);
        }
        return 0;

}

