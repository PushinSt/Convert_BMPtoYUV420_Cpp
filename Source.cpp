
#include "Header.h"

using namespace std;


//расчет Y U V для [begin;end) пикселей
void BMP_to_YUV420(BMP_my bmp, int* yuvMass, int begin, int end) 
{
    int len = bmp.h2.biHeight * bmp.h2.biWidth;
    int a = bmp.h2.biWidth / 2;  //4
    bool ff = false;
    int pos = 0;
    int Y = 0;
    int U = 0;
    int V = 0;

    for (int i = begin; i < end; i++)
    {

        //заполняем i пиксель
        for (int j = 0; j < 4; j++) //4:2:0 квадрат
        {
            ff = 0;
            switch (j)
                {
                case 0:
                    pos = (i / a) * bmp.h2.biWidth + 2 * i; //(0;0)
                    break;
                case 1:
                    pos = (i / a) * bmp.h2.biWidth + 2 * i + 1; //(0;1)
                    break;
                case 2:
                    pos = (i / a + 1) * bmp.h2.biWidth + 2 * i; //(1;0)
                    break;
                case 3:
                    pos = (i / a + 1) * bmp.h2.biWidth + 2 * i + 1; //(1;1)
                    ff = true;
                    break;
                default:
                    pos = 0;
                }

            //расчет YUV
            bmp.calc(pos, Y, U, V);
       
            //Установка на свои места
            yuvMass[pos] = Y;
            yuvMass[len + i] += U;
            yuvMass[len * 5 / 4 + i] += V;

            if (ff) { //Если собрали квадрат, то находим среднее для U и V.
                yuvMass[len + i] = yuvMass[len + i] / 4;
                yuvMass[len * 5 / 4 + i] = yuvMass[len * 5 / 4 + i] / 4;
            }
        }
    }
}

// Добавление YUV картинки в YUV видео
// image_h, image_w  - высота картинки, ширина картинки
// video_h, video_w - высота видео, ширина видео
// video_fr - количество кадров в видеоряде
int video(int image_h, int image_w, int video_h, int video_w, int video_fr)
{
    char video_in[] = "video_input.yuv";
    char video_out[] = "video_output.yuv";
    char image_out[] = "image_output.yuv";


    //размер картинки и видео
    int i_len = image_h * image_w;
    int v_len = video_h * video_w;
    int page = 0;
    int S1 = 0;
    int S2 = 0;

    
    FILE* f_temp = fopen(video_in, "r+b"); //Открываем файл с видео YUV
    FILE* f_new = fopen(video_out, "w+b"); //Создаем файл для нового видео YUV
    
    //Переписываем оригинал видео в новый файл
    unsigned char* mass = new unsigned char[v_len * 3 / 2 * video_fr];
    fread(mass, 1, v_len * 3 / 2 * video_fr, f_temp);
    fwrite(mass, 1, v_len * 3 / 2 * video_fr, f_new);
    delete[] mass;
    fclose(f_temp);


    //Считываем картинку
    f_temp = fopen(image_out, "r+b");
    mass = new unsigned char[i_len * 3 / 2];
    fread(mass, i_len * 3 / 2, 1, f_temp);
    fclose(f_temp);
    

    //Указатель на данные картинки
    unsigned char* q = mass;
    
    //Следуем по кадрам видео
    for (int k = 0; k < video_fr; k++)
    {
        page = v_len * k * 3 / 2; //Позиция в файле начала очередного кадра

        //Следуем по строкам в картинке
        for (int j = 0; j < image_h; j++)
        {
            fseek(f_new, page + j * video_w, 0); //Перемещаемся на первую(следующую) строку в кадре
            q = mass + image_w * j; //Получаем нужную позицию (Y) в картинке
            fwrite(q, sizeof(unsigned char), image_w, f_new); //пишем Y строку картинки в видеокадр

            if (j < image_h / 2)
            {
                S1 = video_w / 2 * j; //Позиция начала UV в видеокадре
                S2 = image_w / 2 * j; //Позиция начала UV в картинке
                
                fseek(f_new, page + v_len + S1, 0); // Перемещаемся на позицию U в видеокадре
                q = mass + i_len + S2; //Получаем нужную позицию (U) в картинке
                fwrite(q, sizeof(unsigned char), image_w / 2, f_new); //пишем U строку картинки в видеокадр


                fseek(f_new, page + S1 + v_len * 5 / 4, 0); // Перемещаемся на позицию V в видеокадре
                q = mass + S2 + i_len * 5 / 4; //Получаем нужную позицию (V) в картинке
                fwrite(q, sizeof(unsigned char), image_w / 2, f_new);  //пишем V строку картинки в видеокадр
            }
        }
    }
    
    fclose(f_new);
    delete[] mass;
    return 1;
}



int main()
{
    BMP_my bmp;

    char image_in[] = "image_input.bmp";
    char image_out[] = "image_output.yuv";
    int video_h = 288; //высота видео
    int video_w = 352; //ширина видео
    int video_fr = 150; //количество кадров в видеоряде
  

    //открытие BMP файла
    FILE* file = fopen(image_in, "r+b");
    fseek(file, 0, SEEK_SET); //Перемещаемся на начало файла
    

    bmp.Set_FileH(file); //Считываем первый заголовок bmp файла
    bmp.Set_InfoH(file); //Считываем второй заголовок bmp файла

    bmp.linePadding = (4 - (bmp.h2.biWidth * 3) % 4) & 3; //Рассчитываем отступ (Строка в bmp файле должна быть кратна 4)

    //Проверка на коллизию по ширине (если истина, то увеличиваем ширину картинки)
    bmp.flag1 = 0;
    if (bmp.linePadding % 2) {
        bmp.h2.biWidth = bmp.h2.biWidth + 1;
        bmp.flag1 = 1;
    }

    //проверка на коллизию по высоте (если истина, то увеличиваем высоту картинки)
    bmp.flag2 = 0;
    if (bmp.h2.biHeight % 2) {
        bmp.h2.biHeight = bmp.h2.biHeight + 1;
        bmp.flag2 = 1;
    }


    //Итоговый размер картинки
    int len = bmp.h2.biHeight * bmp.h2.biWidth;

    //Инициализируем память для YUV формата
    int* yuvMass = new int[len * 3 / 2];
    for (int i = 0; i < len * 3 / 2; i++)
    {
        yuvMass[i] = 0;
    }

    //Считываем пиксели BMP файл
    bmp.Create_Data(file);
    fclose(file);

    //количество квадратов 4:2:0
    int count_square = len / 4;
    
    //получаем количество доступных потоков
    int count_th = std::thread::hardware_concurrency();

    //создаем вектор из потоков
    std::vector<std::thread> threads;
    //Идея для распараллеливания:
    // равномерно распределить квадраты YUV 4:2:0 между потоками
    

    int sum_th = count_th; //количество доступных потоков
    int sum_sq = count_square; // количество "незанятых" квадратов
    int beg = 0;
    int en = 0;
    int step = 0;
    //инициализация каждого потока
    for (int i = 0; i < count_th; i++)
    {
        //определяем шаг расчета
        step = sum_sq / sum_th;
        beg = i * step;
        en = (i + 1) * step;

        sum_th = sum_th - 1; //пересчитываем количество доступных  потоков
        sum_sq = sum_sq - step; //пересчитываем количество "незанятых" квадратов
        
        //BMP_to_YUV420(bmp, yuvMass, beg, en);
        threads.push_back(std::thread(BMP_to_YUV420, bmp, yuvMass, beg, en)); //запускаем очередной поток и указываем ему данные для расчета
    }
    
    //ожидаем завершения всех потоков
    for (auto& t : threads)
        t.join();
    
    //Выводим YUV картинку в файл
    file = fopen(image_out, "w+b");
    fseek(file, 0, SEEK_SET);
    for (int j = 0; j < bmp.h2.biWidth * bmp.h2.biHeight * 3 / 2; j++)
        fwrite(&yuvMass[j], sizeof(unsigned char), 1, file);
    fclose(file);
    printf("BMP -> YUV420 conversion completed. Final image size:\n%ld x %ld\n", bmp.h2.biWidth, bmp.h2.biHeight);


    delete[] yuvMass;


    //Запускаем наложение картинки на видеоряд
    video(bmp.h2.biHeight, bmp.h2.biWidth, video_h, video_w, video_fr);

    return 1;
}



