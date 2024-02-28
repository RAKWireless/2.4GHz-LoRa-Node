# Changelog  

# [1.0.4.1] 2024-2-28
## Added  
- formatted uart print data

# [1.0.4] 2023-11-4
## Added  
- AT+NWM - get or set the network working mode (0 = P2P_LORA, 1 = LoRaWAN)
- AT+PFREQ - configure P2P Frequency (2400000000 - 2500000000)
- AT+PTP - configure P2P TX power (Max 13)
- AT+PSF - configure P2P Spreading Factor (1-8 SF5-SF12)
- AT+PBW - configure P2P Bandwidth (3-BW200 4-BW400 5-BW-800 6-BW1600)
- AT+PCR - configure P2P Code Rate (0-CR4/5 1-CR4/6 2-CR4/7 3-CR4/8)
- AT+PPL - configure P2P Preamble Length
- AT+PSEND - send data in P2P mode
- AT+PRECV - continuous receive mode (AT+PRECV=0 exit receive mode)


# [1.0.3] 2023-10-6  
## Added   
- AT+HWMODEL Get the string of the hardware mode
- AT+HWID  Get the string of the hardware ID
- AT+NJM Get or set the network join mode (0 = ABP, 1 = OTAA)
- AT+DEVADDR Get or set the device address (4 bytes in hex)
- AT+NWKSKEY Get or set the network session key (16 bytes in hex)
- AT+APPSKEY Get or set the application session key (16 bytes in hex)
- AT+NJS Get the join status (0 = not joined, 1 = joined),
- AT+CFM Set/Get comfirm mode
- AT+CFS Get the confirmation status of the last AT+SEND (0 = failure, 1 = success)
- AT+RETY Set/Get the number of retransmissions of Confirm packet data (1-15)


# [1.0.2.4] 2023-9-7  
## Added   
- Increase the MCU crystal calibration, improve the stability of the system.  

# [1.0.2.3] 2023-9-4  
## Added   
- 1.Added the setting of timer compensation
## Fixed  
- 1.Change the example join logic to prevent conflicts with p2p test mode

# [1.0.2.2] - 2023-8-29  
## Added   
- Add AT+INTERVAL，Set sensor data uplink interval time
## Fixed  

# [1.0.2.1] - 2023-8-29  
## Added   
- Add a three-axis sensor, RAK1904, which automatically and continuously sends sensor data after join the network
- Fix classA downlink
## Fixed  

# [1.0.1] - 2023-7-30  
## Added   
AT+CLASS - Set/Get class (0-CLASSA 2-CLASSC)
AT+DR - Set/Get datarate (0-5)
AT+TCONF - Set/Get RF test config
AT+TTX - RF test tx 
AT+TRX - RF test rx continuously receive mode
AT+TRXNOP - RF test terminate an ongoing continuous rx mode 
## Fixed  

# [1.0.0] - 2023-7-9  
## Added   
Added AT instruction frame
## Fixed  

