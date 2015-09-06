#ProbeCube

##專案簡介
ProbeCube是個基於開放硬體的物聯網專案，動手製作WiFi連網的機器來偵測空氣品質並組織共同的觀測地圖。

##運作原理
感測器-Arduino-(wifi)-Thingspeak-觀測地圖

##感測項目
* 溫度
* 濕度
* 空氣中總揮發物質(VoCs)
* 懸浮微粒

##如何開始
1. 準備材料
2. 焊接或使用麵包板連接各部元件組成ProbeCube
3. 註冊[Thingspeak](https://thingspeak.com/)帳號並設定上傳channel
4. 將程式碼客製修改後編譯寫入ProbeCube
5. ProbeCube的資料將會上傳至Thingspeak
6. 到ProbeCube共同觀測地圖提交你的Thindspeak Channel ID分享資料(正在開發中)

##連結

* 詳細組裝及設定步驟圖文
* [共同觀測地圖](http://www.3203.info/Air/)及其[原始碼](https://github.com/immortalmice/ThingSpeak-Visual-Map)

##範例
* [Thingspeak channel 範例](https://thingspeak.com/channels/26769) - 即時上傳的空氣觀測資料!
* 由arduino uno為主板組裝後的ProbeCube  
![](https://github.com/Lafudoci/ProbeCube/blob/master/pc_shield_demo.jpg)
