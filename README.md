# esp32-learn

### programming steps    
1. press BOOT
2. Press EN
3. Release BOOT
4. Release EN


### arduino.json 
``` json
{
    "port": "COM6",
    "output": "../build",
    "board": "esp32:esp32:esp32doit-devkit-v1",
    "configuration": "FlashFreq=80,UploadSpeed=921600,DebugLevel=none",
    "programmer": "ArduinoISP",
    "sketch": "GATT-server\\gatt-server.ino"
}
```
### setting.json
```json
{
    "workbench.colorTheme": "Visual Studio Light",
    "git.autofetch": true,
    "latex-workshop.view.pdf.viewer": "tab",
    "git.confirmSync": false,
    "arduino.ignoreBoards": [
        "WeMos D1"
    ],
    "arduino.additionalUrls": [
        "",
        "http://arduino.esp8266.com/stable/package_esp8266com_index.json",
        "https://raw.githubusercontent.com/jantje/arduino-esp32/master/package/package_Espressif_esp32_index.json",
        "https://dl.espressif.com/dl/package_esp32_index.json"
        ],
    "arduino.logLevel": "verbose"
}
```