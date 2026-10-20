int __klee_errno = 0; /* errno is 0 at the start of the program */
int *__errno_location(void)
{
	/* we don't support multi-threaded programs,
	 * so we can have just this one errno */
	return &__klee_errno;
}
