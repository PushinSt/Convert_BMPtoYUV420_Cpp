#include <stdio.h>
#include <thread>
#include <stdlib.h>
#include <iostream>
#include <vector>

#include <immintrin.h>

//Матрица для расчета YUV коэффициентов
float aa[4 * 3] = { 0.114, 0.587, 0.299, 0,
					0.500, -0.331, -0.169, 128,
					-0.081, -0.419, 0.500, 128};

//Функция расчета YUV коэффициентов  на основе SMID
void  mul_simd(const float* b, float* r)
{
	//объявляем переменные с автоматическим выравниванием
	__m128 line = _mm_load_ps(&b[0]); // считываем значения цветов текущего пикселя  
	__m128 p;
	__m128 s;
	
	for (int i = 0; i < 3; i++)
	{
		p = _mm_setzero_ps(); //обнуляем переменную
		s = _mm_setzero_ps(); //обнуляем переменную
		
		p = _mm_mul_ps(_mm_load_ps(&aa[4*i]), line); //получаем строку коэффициентов (Y,U,V) и умножаем на цвета текущего пикселя
		// p = a1 a2 a3 a4
		// s = 0 0 0 0

		s = _mm_add_ps(s, p); //s=s+p
		// p = a1 a2 a3 a4
		// s = a1 a2 a3 a4

		p = _mm_movehl_ps(p, s); //копирование старшей половины в младшую
		// p = a3 a4 a3 a4
		// s = a1 a2 a3 a4

		s = _mm_add_ps(s, p); //s=s+p
		// p = a3 a4 a3 a4
		// s = a1+a3 a2+a4 a3+a3 a4+a4

		p = _mm_shuffle_ps(s, s, _MM_SHUFFLE(1, 1, 1, 1)); //копирование 1 элемента из s во все элементы p
		// p = a2+a4 a2+a4 a2+a4 a2+a4
		// s = a1+a3 a2+a4 a3+a3 a4+a4

		s = _mm_add_ss(s, p); // s = s + p
		// p = a2+a4 a2+a4 a2+a4 a2+a4
		// s = a1+a3+a2+a4 a2+a4+a2+a4 a2+a4+a3+a3 a2+a4+a4+a4 , в нулевом элементе искомый результат!

		_mm_storeu_ps(r+i, s); // копирование в r+i результат вычисление коэффициента
	}

}


class BMP_my {
public:
	struct File_Header //Структура первого заголовка BMP файла
	{
		short	bfType;
		long	bfSize;
		short	bfReserved1;
		short	bfReserved2;
		long	bfOffBits;
	};


	struct Info_Header //Структура второго заголовка BMP файла
	{
		long	biSize;
		long	biWidth;
		long	biHeight;
		short	biPlanes;
		short	biBitCount;
		long	biCompression;
		long	biSizeImage;
		long	biXPelsPerMeter;
		long	biYPelsPerMeter;
		long	biClrUsed;
		long	biClrImportant;
	};

	File_Header h1; //Первый заголовок
	Info_Header h2; //Второй заголовок

	int linePadding; //Отступ, если длина картинки не кратна 4
	bool flag1, flag2; //Флаги, показывающие что исходный файл имеет нечетные габариты (flag1 - ширина, flag2 - высота)

	int* data; //масив для хранения всех цветов файла
	
	// Чтение bmp файла (цветов) - заполнение массива с верхнего правого угла
	int* Create_Data(FILE* f1)
	{
		data = new int[h2.biWidth * h2.biHeight * 3]; // Создаем массив для хранения пикселей bmp файла
		char temp[3]; //строка для мусора (нули в конце строк bmp файла)
		unsigned char color;
		for (int j = h2.biHeight-1; j >= 0; j--)
		{
			for (int i = 0; i < h2.biWidth; i++)
			{

				if ((i == h2.biWidth / 2) && (flag1)) //Если была коллизия по ширине и находимся сейчас в середине файла (по горизонтали)
				{
					fseek(f1, -3, SEEK_CUR); // то, смещаемся на 3 байта (1 rgb) назад
					//Способ решения коллизии по ширине - дублирование в середину столбца пикселей
				}

				//Считываем пиксели
				fread(&color, sizeof(unsigned char), 1, f1);
				data[3 * h2.biWidth * j + 3 * i] = color;
				fread(&color, sizeof(unsigned char), 1, f1);
				data[3 * h2.biWidth * j + 3 * i + 1] = color;
				fread(&color, sizeof(unsigned char), 1, f1);
				data[3 * h2.biWidth * j + 3 * i + 2] = color;
			}
			fread(temp, sizeof(unsigned char), linePadding, f1); //Читаем лишние символы в конце строки, если они есть

			if ((flag2) && (j == h2.biHeight / 2 )) //Если была коллизия по высоте и находимся в середине файла (по вертикали)
			{
				int q = 0;
				if (flag1)
					q = 3 * (h2.biWidth - 1) + linePadding;
				else
					q = 3 * (h2.biWidth) + linePadding;


				fseek(f1, -1 * q, SEEK_CUR); //  то, смещаемся на одну строку назад
				//Способ решения коллизии по высоте - дублирование в середину строки пикселей
			}
		}
		return data;
	}


	void Set_FileH(FILE* f1) //Считывание первого заголовка
	{
		short a;
		int b;

		fread(&a, sizeof(a), 1, f1);
		this->h1.bfType = a;
		fread(&b, sizeof(b), 1, f1);
		this->h1.bfSize = b;
		fread(&a, sizeof(a), 1, f1);
		this->h1.bfReserved1 = a;
		fread(&a, sizeof(a), 1, f1);
		this->h1.bfReserved2 = a;
		fread(&b, sizeof(b), 1, f1);
		this->h1.bfOffBits = b;
	}

	void Set_InfoH(FILE* f1) //Считывание второго заголовка
	{
		short a;
		int b;

		fread(&b, sizeof(b), 1, f1);
		this->h2.biSize = b;
		fread(&b, sizeof(b), 1, f1);
		this->h2.biWidth = b;
		fread(&b, sizeof(b), 1, f1);
		this->h2.biHeight = b;
		fread(&a, sizeof(a), 1, f1);
		this->h2.biPlanes = a;
		fread(&a, sizeof(a), 1, f1);
		this->h2.biBitCount = a;
		fread(&b, sizeof(b), 1, f1);
		this->h2.biCompression = b;
		fread(&b, sizeof(b), 1, f1);
		this->h2.biSizeImage = b;
		fread(&b, sizeof(b), 1, f1);
		this->h2.biXPelsPerMeter = b;
		fread(&b, sizeof(b), 1, f1);
		this->h2.biYPelsPerMeter = b;
		fread(&b, sizeof(b), 1, f1);
		this->h2.biClrUsed = b;
		fread(&b, sizeof(b), 1, f1);
		this->h2.biClrImportant = b;
	}


	// Расчет составляющих Y U V
	void calc(int pos, int &Y, int &U, int &V) 
	{
		//массивы для СЛАУ, bb - цвета пикселя, cc - расчетные YUV
		float* bb = new float[4] {(float)data[pos * 3], (float)data[pos * 3 + 1], (float)data[pos * 3 + 2], 1.0}; 
		float* cc = new float[4] {0.0, 0.0, 0.0, 0.0};  

		//Рассчет YUV
		mul_simd(bb, cc);

		//Получение ответа
		Y = cc[0];
		U = cc[1];
		V = cc[2];

		delete[] bb, cc;
	}
};



