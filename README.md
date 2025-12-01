# PS/2 keyboard converter implement on RP2040/FreeRTOS

## What is this?(コレは何？)
- RP2040による、PS/2キーボードコンバータの実装例です。状況によってはデバッグが面倒になりそうなので、FreeRTOSを噛ませてみよう、という企みです。


## Environment(動作環境)
Tool                    | Description
:-----------------------|:---------------------------------------------------
**target**              | RP2040(pico, pico_w, etc ...)
**pico-sdk**            | [raspberrypi/pico-sdk](https://github.com/raspberrypi/pico-sdk)
**FreeRTOS**            | FreeRTOSv202406.01-LTS [FreeRTOS/FreeRTOS](https://github.com/FreeRTOS/FreeRTOS)


## Status(進捗状況)
- 現在は、spi_slave.c内部のISRで、PS/2キーボードからのコードを読んでBuffer[]に格納、buffer_taskでBuffer[]にデータが追加されていればSerial0に出力しています。SPI側は受信割込で、FreeRTOS側と非同期に動作させています。
- メッセージキュー部分はデバッグ中です。いまの所は必要ないんですけどね。


## Hint(読解のヒント)
- 大したhackじゃないけど。。。spi_slave.cのreenable_spi()では、SPI受信を禁止→有効にしています。外部からSSPFSSINが来ないので、そのままでは次の受信が出来ません。RP2040データシートの”4.5.3.9. Motorola SPI frame format”のFigure 89も見てね。


## TODO(今後の話)
- 現状、spi_slave.cやbuffer.c自体はthread safe＊ではない＊。thread safetyにするべき。
    - →具体的には、全ての動的変数についてpvPortMalloc()/vPortMfree()を使う、べき。
    - →明らかにmemmang/heap_*.cを理解するべきであり、わかんない人は触らない方が吉。
- 公式の回路図が必要？("picotool info -bp *.uf2"で十分なのでは？)
    - →CLKとDATに、直列に150Rが入っているだけ。CLKはpullup＊していない＊。
- PS/2キーコードのパリティチェックは、たぶん必要。今は何も＊していない＊。
    - →パリティエラーの際の処理が未定義。読み飛ばすしかない、とは思うけど。
- USBキーボード対応は、他の人にお任せします(私は、やりません。期待しないで下さい)。


## LICENSE(ライセンス)
- ソースをbuildするには、当然ですがpico-sdk [raspberrypi/pico-sdk](https://github.com/raspberrypi/pico-sdk)が必要です。spi_slave.cは、pico-examples [raspberrypi/pico-examples](https://github.com/raspberrypi/pico-examples)内のコードを参考に(ほぼパクり)させて頂きました。

- ソースをbuildするには、FreeRTOS-Kernel(FreeRTOSv202406.01-LTS以降)が必要です。
[FreeRTOS/FreeRTOS](https://github.com/FreeRTOS/FreeRTOS)

- 私(yasunoxx▼Julia)が書いたプログラムは、MITライセンスで開示しています。本プログラム [ps_2_freertos](https://github.com/yasunoxx/ps_2_freertos)を使用した/使用しない事による全ての結果について、上記権利者と私は何の保証も賠償も致しません。
