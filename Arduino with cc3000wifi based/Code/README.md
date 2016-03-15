使用說明:<br>
1. 安裝ch341驅動程式<br>
2. 匯入liberies內zip檔案至arduino IDE<br>
3. 將對應版本程式碼(配有不同module的機種)複製貼上至arduino開發編輯器<br>
4. 修改程式碼中以下冒號內數值後即可上傳至arduino板子:<br>
    API write key<br>
    wifi帳號與密碼<br>
    GP2Y1010校正參數 (G3機種不必填)<br>
<br>
注意: 程式碼內包含WDT的重置方法，若使用arduinonano的板子建議[先刷optiboot的韌體]<br>
(http://lafudo.blogspot.tw/2015/02/arduino-nanoloaderoptibootwatch-dog.html)<br>
<br>
v0.98 changes:<br>
調換宣告結構順序方便更改參數<br>
更改計時的方式由delay改為精準millis計時觸發上傳動作<br>
更改等待DHCP的方式，若5秒內無法取得IP則重新啟動wifi晶片<br>
移除警告燈號的功能<br>
<br>
v0.99b0223 changes:<br>
新增DHCP冷卻時間機制避免某些路由器issue<br>
<br>
v0.99b0224 changes:<br>
取得thingspeak ip的動作改為僅在開機要求一次<br>
http get後讀回thingspeak的回傳字串<br>
加入connect close<br>
