#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>



/*----------------------------------------------------------
Функция get_files_count - рекурсивная. Возвращает количество
файлов по заданной директории и поддиректориях.
----------------------------------------------------------*/
int get_files_count(char* curr_dir)
{
	// количество файлов
	int files_count = 0;

	// открываем директорию
	DIR* dirs = opendir(curr_dir);
	if (dirs)
	{
		// указатель на структуру для хранения информации о директории
		struct dirent* dirstr;

		// читаем информацию о файлах и директориях в текущей директории
		while (dirstr = readdir(dirs))
		{
			// получение полного имени
			char* file_name = dirstr->d_name;
			char* full_name = (char*)alloca(strlen(curr_dir) + strlen(file_name) + 1);
			strcpy(full_name, curr_dir);
			strcat(full_name, "/");
			strcat(full_name, file_name);

			// структура для хранения информации о файле
			struct stat filestr;

			// если файл, увеличиваем счетчик, иначе - рекурсивно заходим в директорию и повторяем действия
			stat(full_name, &filestr);
			if (S_ISREG(filestr.st_mode) && !S_ISLNK(filestr.st_mode))
				files_count++;
			else
				if (S_ISDIR(filestr.st_mode) && strcmp(file_name,".") && strcmp(file_name,"..") && !S_ISLNK(filestr.st_mode))
					files_count += get_files_count(full_name);
		}
		// закрываем директорию
		closedir(dirs);
	}
	else
		printf("Couldn't open directory - %s\n", curr_dir);

	return files_count;
}



/*---------------------------------------------------------------------------------
Функция get_list - рекурсивная, на переданный указатель записывает список имен
файлов по заданому имени каталога, а также заполняет массив размеров каждого файла.
---------------------------------------------------------------------------------*/
void get_list(int* files_count, char** files_list, char* curr_dir, int* files_size)
{
	// открываем директорию
	DIR* dirs = opendir(curr_dir);
	if (dirs)
	{
		// указатель на структуру для хранения информации о директории
		struct dirent* dirstr;

		// читаем информацию о файлах и директориях в текущей директории
		while (dirstr = readdir(dirs))
		{
			// получение полного имени
			char* file_name = dirstr->d_name;
			char* full_name = (char*)alloca(strlen(curr_dir) + strlen(file_name) + 1);
			strcpy(full_name, curr_dir);
			strcat(full_name, "/");
			strcat(full_name, file_name);

			// структура для хранения информации о файле
			struct stat filestr;

			// если файл, пишем данные о нем, иначе - рекурсивно заходим в директорию и повторяем действия
			stat(full_name, &filestr);
			if (S_ISREG(filestr.st_mode) && !S_ISLNK(filestr.st_mode))
			{
				strcpy(files_list[*files_count],full_name);
				*(files_size + *files_count) = filestr.st_size;
				(*files_count)++;
			}
			else
				if (S_ISDIR(filestr.st_mode) && strcmp(file_name,".") && strcmp(file_name,"..") && !S_ISLNK(filestr.st_mode))
					get_list(files_count, files_list, curr_dir, files_size);
		}
		// закрываем директорию
		closedir(dirs);
	}
	else
		printf("Couldn't open directory - %s\n", curr_dir);
}



/*--------------------------------------------------------------
Функция создает новый процесс, в котором сравниваются два файла.
Входные параметры: имя первого и второго файла и их размер.
--------------------------------------------------------------*/
int create_fork(char *file_name1, char *file_name2, int *file_size)
{
	//создается новый дочерний процесс
	int new_pid = fork();

	// если процесс ребенок
	if (new_pid == 0)
	{
		int curr_size = 0;
		// открываем файлы и сравниваем содержимое посимвольно
		FILE* file_content1 = fopen(file_name1, "r");
		FILE* file_content2 = fopen(file_name2, "r");
		while (abs(fgetc(file_content1)) == fgetc(file_content2)) curr_size++;

		// выводим строку с результатом
		printf("\n---------- PID: %d ----------\nFile 1: %s\nFile 2: %s\nResult: %d of %d bytes match\n\n",
			getpid(), file_name1, file_name2, curr_size, *file_size);

		// закрываем файлы
		fclose(file_content1);
		fclose(file_content2);
	}
	
	return new_pid;
}



/*----------------------------------------------------------------
Главная функция. Входные параметры получаем из командной строки.
Параметры: пути к директориям и максимальное количество процессов.				
----------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	// инициализация стартовых параметров
	if (argc != 4)
	{
		puts("Put that info in argv: DIR1 DIR2 N");
		exit(1);
	}
	char* dir_name1 = argv[1];
	char* dir_name2 = argv[2];
	int N;
	if (!sscanf(argv[3],"%d",&N))
	{
		puts("N - isn't a number");
		exit(1);
	}

	// список файлов в директориях и поддиректориях
	char** files_list1;
	char** files_list2;

	// массивы размеров файлов
	int* files_size1;
	int* files_size2;

	// количество файлов в двух директориях
	int files_count1 = get_files_count(dir_name1);
	int files_count2 = get_files_count(dir_name2);
	printf("\nTotal files found:  %d ", files_count1 + files_count2);

	// выделение памяти
	files_list1 = malloc(files_count1 * (sizeof(char*)));
	files_list2 = malloc(files_count2 * (sizeof(char*)));
	files_size1 = malloc(files_count1 * (sizeof(int)));
	files_size2 = malloc(files_count2 * (sizeof(int)));
	int i = 0;
	while (i < files_count1)
	{
		*(files_list1 + i) = malloc(512 * sizeof(char));
		i++;
	}
	i = 0;
	while (i < files_count2)
	{
		*(files_list2 + i) = malloc(512 * sizeof(char));
		i++;
	}

	// обнуление размеров
	files_count1 = 0;
	files_count2 = 0;
	
	// получение списка имен и размеров файлов
	get_list(&files_count1, files_list1, dir_name1, files_size1);
	get_list(&files_count2, files_list2, dir_name2, files_size2);
	printf("Total files read:  %d\n\n", files_count1 + files_count2);
	
	i = 0;
	int parent_proc = 1;
	int k, n = 0;

	// цикл перебора всех файлов и сравнение файлов с одинаковыми размерами
	while (i < files_count1)
	{
		k = 0;
		while (k < files_count2)
			if (parent_proc > 0)
			{
				if (*(files_size1 + i) == *(files_size2 + k))
				{
					n++;
					if (n > N)
					{
						wait(NULL);
						n--;
					}
					printf("Current processes count: %d\n", n);
					parent_proc = create_fork(*(files_list1 + i), *(files_list2 + k), files_size1 + i);
				}
				k++;
			}
			else break;
		i++;
	}

	// ожидание завершения оставшихся процессов
	if (parent_proc > 0)
		while(wait(NULL) > 0)
		{
			n--;
			printf("Current processes count: %d\n", n);
		}

	// освобождение памяти
	i = 0;
	while (i < files_count1)
	{
		free(*(files_list1 + i));
		i++;
	}
	i = 0;
	while (i < files_count2)
	{
		free(*(files_list2 + i));
		i++;
	}
	free(files_list1);
	free(files_list2);
	free(files_size1);
	free(files_size2);

	return 0;
}

