<img src="/pc_logo/probecube logo.png" width="250"> 

## 專案簡介
ProbeCube是個基於開源硬體的物聯網專案，鼓勵大眾動手製作WiFi連網的機器來偵測空氣品質並組織共同的觀測地圖。

## 感測器開發方向原則
* 零件需容易取得
* 低組裝與操作門檻
* 相容於MIT開放原始碼授權

## 運作原理
空氣汙染 -> 感測器 -> Arduino(wifi) -> Thingspeak -> 觀測地圖

## 可相容的控制板
* Arduino NANO, UNO + Adafruit CC3000 wifi module
* Particle Photon

## 可相容的感測器
* 溫度/濕度 - DHT22
* 空氣中總揮發物質(VoCs) - FIGARO tgs2602
* 懸浮微粒PM2.5 - PMS3003(G3) 或 GP2Y1010AU0F
* 輻射劑量 - pockect gieger

## PCB設計附加規格
* 全腳位預留輸出排母
* I2C預留接座x2
* groove 預留接座x1

## 資料呈現方式
* 行動裝置: [Blynk app](http://www.blynk.cc/) (詳見範例)
* 網頁: Thingspeak, g0v零時空污觀測網 (詳見範例)

## 外殼
* 3D 列印外殼
* 自行挖洞加工收納盒 ( ex. 大創$39x2)

## 如何開始
1. 準備材料 (材料表)
2. 在PCB上焊接或使用麵包板連接各部元件組成ProbeCube
3. 註冊[Thingspeak](https://thingspeak.com/)帳號並設定上傳之頻道(channel)
4. 將程式碼依需要修改後編譯寫入ProbeCube
5. ProbeCube的資料將會上傳至Thingspeak
6. 到ProbeCube共同觀測地圖提交你的Thindspeak Channel ID分享資料(已併入g0v專案，請參考連結)

## 範例
* 麵包版接線示意

<img src="https://github.com/Lafudoci/ProbeCube/blob/master/Particle Photon based/ProbeCube_v099_20160225_bb.png" width="500">


* 可洗PCB幫助縮小裝置體積

<img src="/pc_pics/20160310.JPG" width="500">

* 由arduino uno為搭配已洗好PCB組裝後的ProbeCube  

<img src="/pc_pics/pc_uno_shield_demo.jpg" width="500">

* 用大創盒子挖洞加工成外殼  

<img src="/pc_pics/handmadecase.jpg" width="500">


* 由Particle Photon為主板組裝後的ProbeCube裝上3D print外殼搭配Blynk app的呈現

<img src="/pc_pics/pc_photon_blynk_demo.jpg" width="500">

* 由Particle Photon為主板組裝後的ProbeCube裝上3D print外殼

<img src="/pc_pics/IMG_6577.jpg" width="500">


* [Thingspeak channel 範例](https://thingspeak.com/channels/26769) - 即時上傳的空氣觀測資料!

<img src="/pc_pics/ts_demo.JPG" width="500">

* g0v零時空汙觀測網的即時呈現

<img src="/pc_pics/20200408T08.jpg" width="500">

## 連結

* [詳細組裝及設定步驟圖文](https://probecube.hackpad.com/-ProbeCube-fUI4k1BnZQ2)
* [g0v零時空汙觀測網專案](https://beta.hackfoldr.org/g0vairmap)
