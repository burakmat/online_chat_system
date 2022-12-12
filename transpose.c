#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <pthread.h>
#define BUFFER_SIZE 1

typedef struct s_data
{
	char ***array;
	int len;
	char ***output;
}	t_data;

static int	wordnum(const char *s, char c)
{
	int	i;
	int	num;
	int	sp;

	i = 0;
	num = 0;
	while (s[i])
	{
		while (s[i] && s[i] == c)
			i++;
		sp = 1;
		while (s[i] && s[i] != c)
		{
			if (sp == 1)
			{
				num++;
				sp = 0;
			}
			i++;
		}
	}
	return (num);
}

static char	*take_wrd(const char *s, char c)
{
	char	*word;
	int		i;

	i = 0;
	while (s[i] && s[i] != c)
		i++;
	word = (char *)malloc(sizeof(char) * (i + 1));
	if (!word)
		return (NULL);
	i = 0;
	while (s[i] && s[i] != c)
	{
		word[i] = s[i];
		i++;
	}
	word[i] = '\0';
	return (word);
}

char	**split(char const *s, char c)
{
	char	**result;
	int		wordnm;
	int		i;

	if (!s)
		return (NULL);
	i = 0;
	wordnm = wordnum(s, c);
	result = (char **)malloc(sizeof(char *) * (wordnm + 1));
	if (!result)
		return (NULL);
	while (*s)
	{
		while (*s && *s == c)
			s++;
		if (*s && *s != c)
		{
			result[i] = take_wrd(s, c);
			i++;
			while (*s && *s != c)
				s++;
		}
	}
	result[i] = NULL;
	return (result);
}

int	string_length(char *str)
{
	int	i;

	i = 0;
	if (!str)
		return (0);
	while (*str++)
		i++;
	return (i);
}

int	check_newline(char *ptr)
{
	if (!ptr)
		return (1);
	while (*ptr)
	{
		if (*ptr == '\n')
			return (0);
		ptr++;
	}
	return (1);
}

char	*append_string(char *str, char *buf)
{
	char	*result;
	int		i;
	int		j;

	if (!buf)
		return (NULL);
	if (!str)
	{	
		str = malloc(1);
		*str = '\0';
	}
	result = malloc(string_length(str) + string_length(buf) + 1);
	if (!result)
		return (NULL);
	i = -1;
	while (str[++i])
		result[i] = str[i];
	j = 0;
	while (buf[j])
		result[i++] = buf[j++];
	result[i] = '\0';
	free(str);
	return (result);
}

char	*get_line(char *str)
{
	char	*line;
	int		i;
	int		j;

	if (!(*str))
		return (NULL);
	i = 0;
	while (str[i] && str[i] != '\n')
		i++;
	if (str[i])
		i++;
	line = malloc(i + 1);
	if (!line)
		return (NULL);
	j = 0;
	while (j < i)
	{
		line[j] = str[j];
		++j;
	}
	line[j] = '\0';
	return (line);
}

char	*reading_from_file(int fd, char *str)
{
	char	*buf;
	int		byte_num;

	byte_num = 1;
	buf = malloc(BUFFER_SIZE + 1);
	if (!buf)
		return (NULL);
	while (check_newline(str) && byte_num)
	{
		byte_num = read(fd, buf, BUFFER_SIZE);
		if (byte_num == -1)
		{
			free(buf);
			return (NULL);
		}
		buf[byte_num] = '\0';
		str = append_string(str, buf);
	}
	free(buf);
	return (str);
}

char	*shift_to_endline(char *str)
{
	char	*shifted_str;
	int		i;
	int		j;

	i = 0;
	while (str[i] && str[i] != '\n')
		++i;
	if (!str[i])
	{
		free(str);
		return (NULL);
	}
	++i;
	shifted_str = malloc(string_length(str) - i + 1);
	if (!shifted_str)
		return (NULL);
	j = 0;
	while (j < string_length(str) - i)
	{
		shifted_str[j] = str[j + i];
		j++;
	}
	shifted_str[j] = '\0';
	free(str);
	return (shifted_str);
}

char	*get_next_line(int fd)
{
	static char	*str;
	char		*line;

	if (BUFFER_SIZE <= 0 || fd < 0)
		return (NULL);
	str = reading_from_file(fd, str);
	if (!str)
		return (NULL);
	line = get_line(str);
	str = shift_to_endline(str);
	return (line);
}

int find_new_line_index(char *str)
{
	int i = 0;

	while (str[i])
	{
		if (str[i] == '\n')
			return (i);
		++i;
	}
	return (-1);
}

void *north_west_handler(void *param)
{
	t_data *data = (t_data *)param;

	int i = 0;
	int j;
	while (i < data->len / 2)
	{
		j = 0;
		while (j < data->len / 2)
		{
			data->output[j][i] = data->array[i][j];
			++j;
		}
		++i;
	}
	return NULL;
}

void *north_east_handler(void *param)
{
	t_data *data = (t_data *)param;

	int nl;
	int i = 0;
	int j;
	while (i < data->len / 2)
	{
		j = data->len / 2;
		while (j < data->len)
		{
			data->output[j][i] = data->array[i][j];
			nl = find_new_line_index(data->array[i][j]);
			if (nl != -1)
				data->array[i][j][nl] = '\0';
			++j;
		}
		++i;
	}
	return NULL;
}

void *south_west_handler(void *param)
{
	t_data *data = (t_data *)param;

	int i = data->len / 2;
	int j;
	while (i < data->len)
	{
		j = 0;
		while (j < data->len / 2)
		{
			data->output[j][i] = data->array[i][j];
			++j;
		}
		++i;
	}
	return NULL;
}

void *south_east_handler(void *param)
{
	t_data *data = (t_data *)param;

	int nl;
	int i = data->len / 2;
	int j;
	while (i < data->len)
	{
		j = data->len / 2;
		while (j < data->len)
		{
			data->output[j][i] = data->array[i][j];
			nl = find_new_line_index(data->array[i][j]);
			if (nl != -1)
				data->array[i][j][nl] = '\0';
			++j;
		}
		++i;
	}
	return NULL;
}

int main()
{
	int fd = open("input_matrix.txt", O_RDONLY);
	if (fd < 0)
	{
		perror("Cannot open file");
		return (1);
	}
	char *line;
	char **numbers;
	char ***array = NULL;
	int len;

	while ((line = get_next_line(fd)) != NULL)
	{
		numbers = split(line, ' ');
		free(line);
		if (array == NULL)
		{
			array = malloc(sizeof(char **) * 2);
			array[0] = numbers;
			array[1] = NULL;
		}
		else
		{
			int i = 0;
			char ***tmp;
			while (array[i])
			{
				++i;
			}
			tmp = malloc(sizeof(char **) * (i + 2));
			i = 0;
			while (array[i])
			{
				tmp[i] = array[i];
				++i;
			}
			tmp[i] = numbers;
			tmp[i + 1] = array[i];
			free(array);
			array = tmp;
			len = i + 1;
		}
	}
	
	close(fd);

	pthread_t ne, nw, se, sw;
	t_data data;
	data.array = array;
	data.len = len;
	data.output = malloc(sizeof(char **) * (len + 1));
	int i = 0;
	while (i < len)
	{
		data.output[i] = malloc(sizeof(char *) * (len + 1));
		data.output[i][len] = NULL;
		++i;
	}
	data.output[len] = NULL;
	pthread_create(&nw, NULL, &north_west_handler, &data);
	pthread_create(&ne, NULL, &north_east_handler, &data);
	pthread_create(&sw, NULL, &south_west_handler, &data);
	pthread_create(&se, NULL, &south_east_handler, &data);

	pthread_join(nw, NULL);
	pthread_join(ne, NULL);
	pthread_join(sw, NULL);
	pthread_join(se, NULL);

	int fdo = open("output_matrix.txt", O_WRONLY | O_CREAT | O_TRUNC, 0777);

	i = 0;
	int j;

	while (i < len)
	{
		j = 0;
		while (j < len)
		{
			write(fdo, data.output[i][j], string_length(data.output[i][j]));
			write(fdo, " ", 1);
			++j;
		}
		write(fdo, "\n", 1);
		++i;
	}
	close(fdo);
}