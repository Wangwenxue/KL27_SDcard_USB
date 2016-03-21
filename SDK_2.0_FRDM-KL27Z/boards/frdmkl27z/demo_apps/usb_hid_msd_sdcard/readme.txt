用的KL26里的SPI驱动，没用DMA，现在SPI速率是12Mbps。
用的mbed协议栈，目前可以实现mSD+HID复合设备，速率250KB/s(15 Block)
如果1block 只有163KB/s的速率

注意：YL_KL26上要供电，但是也要确保KL26上没有程序，不然同时会影响他。


这一版同时完成了HID鼠标的例子。




