int scandir(const char *dir, struct dirent ***namelist,
						int (*select)(const struct dirent *),
						int (*compar)(const struct dirent **, const struct dirent **));