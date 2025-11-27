# PS/2 keyboard converter implement on RP2040/FreeRTOS

## What is this?(コレは何？)
- RP2040による、PS/2キーボードコンバータの実装例です。状況によってはデバッグが面倒になりそうなので、FreeRTOSを噛ませてみよう、という企みです。


## Environment(動作環境)
Tool                    | Description
:-----------------------|:---------------------------------------------------
**target**              | RP2040(pico, pico_w, etc ...)
**pico-sdk**            | [raspberrypi/pico-sdk](https://github.com/raspberrypi/pico-sdk)


## Status(進捗状況)
- 現在は、spi_slave.c内部の無限ループ内で、PS/2キーボードからのコードを読んで、Serial0に出力しています。これをSPI受信割り込みにして、FreeRTOS側と非同期に動作させる予定です。
- FreeRTOS側は、現在動作させていません。動作させていても構わないんですけどね。


## TODO(今後の話)
- 公式の回路図が必要？("picotool info -bp *.uf2"で十分なのでは？)
- PS/2キーコードのパリティチェックは、たぶん必要。今は何もしていない。
- USBキーボード対応は、他の人にお任せします(私は、やりたくない)。


## LICENSE(ライセンス)
- ソースをbuildするには、当然ですがpico-sdk[raspberrypi/pico-sdk](https://github.com/raspberrypi/pico-sdk)が必要です。spi_slave.cは、pico-examples[raspberrypi/pico-examples](https://github.com/raspberrypi/pico-examples)内のコードを参考に(ほぼパクり)させて頂きました。

- ソースをbuildするには、FreeRTOS-Kernel(FreeRTOSv202406.01-LTS)が必要です。
[FreeRTOS/FreeRTOS](https://github.com/FreeRTOS/FreeRTOS)

- 私(yasunoxx▼Julia)が書いたプログラムは、MITライセンスで開示しています。本プログラム[ps_2_freertos](https://github.com/yasunoxx/ps_2_freertos)を使用した/使用しない事による全ての結果について、上記権利者と私は何の保証も賠償も致しません。
