# Тестовое задание (С++)


### Задача:
Открыть файл, содержащий видеоряд (YUV420) и файл BMP, содержащий небольшую картинку (RGB).  
Сделать для картинки преобразование RGB ->YUV420.  
Предполагается, что BMP файл содержит только данные в формате RGB 24 бит/пиксел без альфа канала, без палитры, без компрессии.  
Наложить поверх каждого кадра видеоряда картинку и сохранить результат в выходной файл (YUV420).  
Размер bmp картинки может быть меньше либо равен размеру картинки входного видеоряда.  
Не использовать сторонние библиотеки, только стандартная библиотека языка.  
Бонус 1:
Преобразование RGB->YUV делать в многопоточном режиме, с использованием C++11 threads  
Бонус 2: Оптимизировать преобразование RGB->YUV с помощью набора инструкций SIMD, например Intel SSE2 или AVX2 (intrinsics).  


### Реализация:

Компиляция происходит с помощью стандартной команды g++ (для удобства имеется готовый make файл):
```
g++ -o prog1 Source.cpp Header.h -lpthread
```
После компиляции программа запускается командой: 
```
./prog1
```




### Описание работы программы:
Входные данные:
- Исходное изображение BMP: "image_input.bmp";
- Исходное видео YUV: "video_input.yuv";  (высота видео: 288, ширина видео: 352, количество кадров в видеоряде: 150)

Выходные данные:
- Преобразованное изображение YUV: "image_output.yuv";
- Преобразованное видео: "video_output.yuv";

Используемые библиотеки:
```cpp
#include <stdio.h>
#include <thread>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <immintrin.h>
```

 Для решения задачи используются механизм распараллеливания с помощью потоков. Учитывается максимальное доступное количество потоков для текущей рабочей машины. 
Для оптимизации вычислений были применены инструкций SIMD (SEE)

Алгоритм решения задачи:
1. Считывание BMP файла:
- Считывание первого заголовка;
- Считывание второго заголовка;
- Считывание пикселей (Решение коллизии по высоте(ширине) - если нечетная сторона, то добавляем столбец(строку) в середину файла).
2. Преобразование BMP -> YUV420:
- Подсчет количества квадратов UV: count_square = width * height / 4;
- Получение числа доступных потоков, равномерное распределение числа квадратов UV на все потоки;
- Запуск параллельного расчета коэффициентов YUV. При расчете коэффициентов Y,U,V использовать решение СЛАУ Ax=b, где A - матрица 4x3 коэффициентов, x - очередной пиксель цвета rgb, b - искомые значения Y,U,V. Для решения СЛАУ использовались инструкций SIMD (SEE);
- Сохранения результат преобразования в файл.
3. Наложение картинки YUV420 на видеоряд YUV420:
- Для упрощения задачи решаем, что накладывать картинку будем в левый верхний угол видеоряда;
- Следуем по каждому кадру видеоряда, вставляем байты из картинки в кадр;
- Сохраняем результат наложения в файл.

