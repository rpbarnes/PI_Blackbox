# PI_Blackbox
Contains firmware for Raspberry Pi datalogging 
  - 'GUI/'
      contains bare bones code for C/GTK based GUI to display on HDMI screen

  - 'InfluxDB/'
      working code to store metrics to remote database

  - 'PI_Firmware/'
      code that runs on PI_DAQ board that synchronously takes data samples
      
  - 'Sockets/' 
      sample code for sending text packets between to computers over socket connection
      
  - 'Recipe/' 
      crude version to run stepped loads over defined time intervals on the PI_DAQ
