Докладнше про роботу з апаратурою буде на відповідних лекціях у другій частині
курсу, тут же просто використаємо світлодіоди та кнопку, які є на платі.

У прикладі onboard_io використано давно застарілу, але все ще підтримувану
бібліотеку роботи з портами, тут її достатньо.
Documentation/driver-api/gpio/legacy.rst

Приклад написано дещо «нечесно»
Світлодіоди на платі вже використовуються для індикації стану системи.
Але при нашій роботі з nfs нема звертання до MMC (номер 3) і uSD (1), тому
ніщо не перетре наш вивід на ці два світлодіоди.
Відповідно, ми нахабно не будемо робити запит на використання цих світлодіодів
(однак він поверне "Device or resource busy"), а просто вмикатимемо та вимикатимемо їх.

У наступному домашньому завданні використовуватиметься також кноппка поруч з гніздом uSD-карти.
Вона використовується лише під час завантаження системи і ділить вівід SOC з виходом
на фізичний рівень HDMI. Для використання кнопки у програмі роботу
контролера HDMI необхідно заборонити.

У листі є патч, який слід накласти на am335x-boneblack.dts, перезібрати
та замінити device tree у системі (не забудьте про необхідні ескпорти перед запуском make).
Скопіюйте патч у каталог $KDIR і виконайте наступні команди:

$ git am 0001*.patch
$ make am335x-boneblack.dtb
$ sudo cp arch/arm/boot/dts/am335x-boneblack.dtb /srv/nfs/busybox/boot/

Приклад окрім демонстрації команд вмикання та вимикання світлодіода перевіряє доступність кнопки —
init запалює один з двох світлодіодів залежно від того, чи натиснено кнопку.

