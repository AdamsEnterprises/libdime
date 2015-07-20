/**
 * @file
 * @brief	Internal logging functions. This function should be accessed using the appropriate macro.
 */

#include <core/magma.h>

static uint64_t log_date;
static bool_t log_enabled = true;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief	Disable logging.
 */
void log_disable(void) {
	mutex_lock(&log_mutex);
	log_enabled = false;
	mutex_unlock(&log_mutex);
}

/**
 * @brief	Enable logging.
 */
void log_enable(void) {
	mutex_lock(&log_mutex);
	log_enabled = true;
	mutex_unlock(&log_mutex);
}

/**
 * @brief	Print the current stack backtrace to stdout.
 * @note	This function was created because backtrace_symbols() can fail due to heap corruption.
 * @return	-1 if the backtrace failed, or 0 on success.
 */
int_t print_backtrace() {
	void *buffer[1024];
	char strbuf[1024];
	int_t pipefds[2];
	int_t nbt, nfound = 0, result = 0;
	ssize_t nread;

	nbt = backtrace(buffer, (sizeof (buffer) / sizeof (void *)));

	if (nbt < 0) {
		return -1;
	}

	// Create a pipe to read back the dumped line numbers.
	if (pipe(pipefds) < 0) {
		return -1;
	}

	backtrace_symbols_fd(buffer, nbt, pipefds[1]);

	/* possible error, need to check return value (in gcc 4.6)*/
	if (write(STDOUT_FILENO, "   ", 3) < 0) {
                return -1;
        }

	while (nfound < nbt) {
		nread = read(pipefds[0], strbuf, sizeof(strbuf));

		if (nread < 0) {
			result = -1;
			break;
		} else if (!nread) {
			break;
		}

		for (ssize_t i = 0; i < nread; i++) {
			/* possible error, need to check return value */
                    if (write(STDOUT_FILENO, &strbuf[i], 1) < 0) {
                            return -1;
                    }

			if (strbuf[i] == '\n') {
				nfound++;

				if (nfound != nbt) {
					/* possible error, need to check return value */
                                    if (write(STDOUT_FILENO, "   ", 3) < 0) {
                                            return -1;
                                    }
				}

			}

		}

	}

	fsync(STDOUT_FILENO);
	close(pipefds[0]);
	close(pipefds[1]);

	return result;
}

/**
 * Logs the message provided by @a format. The global configuration dictates whether the @a file, @a function and
 * @a line are also logged. The global configuration can be overridden on a per call basis using the @a options
 * parameter.
 */
void log_internal(const char *file, const char *function, const int line, M_LOG_OPTIONS options, const char *format, ...) {

	time_t now;
//	size_t size;
	va_list args;
	struct tm local;
//	void *array[1024];
	bool_t output = false;
	char /***strings = NULL, */ buffer[128];
	const char *errmsg;

	mutex_lock(&log_mutex);

	// Someone has disabled the log output.
	if (!log_enabled) {
		mutex_unlock(&log_mutex);
		return;
	}

	if ((magma.log.time || M_LOG_TIME == (options & M_LOG_TIME)) && !(M_LOG_TIME_DISABLE == (options & M_LOG_TIME_DISABLE))) {
		now = time(NULL);
		localtime_r(&now, &local);
		strftime(buffer, 128, "%T", &local);

		fprintf(stdout, "%s%s", (output ? " - " : "["), buffer);
		output = true;
	}

	if ((magma.log.file || M_LOG_FILE == (options & M_LOG_FILE)) && !(M_LOG_FILE_DISABLE == (options & M_LOG_FILE_DISABLE))) {
		fprintf(stdout, "%s%s", (output ? " - " : "["), file);
		output = true;
	}

	if ((magma.log.function || M_LOG_FUNCTION == (options & M_LOG_FUNCTION)) && !(M_LOG_FUNCTION_DISABLE == (options & M_LOG_FUNCTION_DISABLE))) {
		fprintf(stdout, "%s%s%s", (output ? " - " : "["), function, "()");
		output = true;
	}

	if ((magma.log.line || M_LOG_LINE == (options & M_LOG_LINE)) && !(M_LOG_LINE_DISABLE == (options & M_LOG_LINE_DISABLE))) {
		fprintf(stdout, "%s%i", (output ? " - " : "["), line);
		output = true;
	}

	if (output)
		fprintf(stdout, "] = ");

	va_start(args, format);
	vfprintf(stdout, format, args);
	va_end(args);

	if (!(M_LOG_LINE_FEED_DISABLE == (options & M_LOG_LINE_FEED_DISABLE))) {
		fprintf(stdout, "\n");
	}

	if ((magma.log.stack || M_LOG_STACK_TRACE == (options & M_LOG_STACK_TRACE)) && !(M_LOG_STACK_TRACE_DISABLE == (options & M_LOG_STACK_TRACE_DISABLE))) {

		if (print_backtrace() < 0) {
			errmsg = "Error printing stack backtrace to stdout!\n";
			/* possible error, need to check return value */
			if (write(STDERR_FILENO, errmsg, ns_length_get(errmsg)) < 0) {
                                mutex_unlock(&log_mutex);
                                return;
                        }
		}

		/*size = backtrace(array, 1024);
		strings = backtrace_symbols(array, size);

		for (uint64_t i = 0; i < size; i++) {
		        fprintf(stdout, "   %s\n", strings[i]);
		}

		if (strings)
		        free(strings); */
	}

	fflush(stdout);
	mutex_unlock(&log_mutex);
}

void log_rotate(void) {

	uint64_t date;
	FILE *orig_out, *orig_err;
	chr_t *log_file = MEMORYBUF(1024);

	if (magma.output.file && magma.output.path && (date = time_datestamp()) != log_date) {

		orig_out = stdout;
		orig_err = stderr;
		log_date = date;

		if (snprintf(log_file, 1024, "%s%smagmad.%lu.log", magma.output.path, (*(ns_length_get(magma.output.path) + magma.output.path) == '/') ? "" : "/",
		             date) >= 1024) {
			log_critical("Log file path exceeded available buffer. { file = %s%smagmad.%lu.log }", magma.output.path,
			             (*(ns_length_get(magma.output.path) + magma.output.path) == '/') ? "" : "/", date);
			return;
		}

		pthread_mutex_lock(&log_mutex);
		if (!(stdout = freopen64(log_file, "a", stdout))) {
			stdout = orig_out;
			log_critical("Unable to rotate the error log. { file = %s }", log_file);
			pthread_mutex_unlock(&log_mutex);
			return;
		}


		if (!(stderr = freopen64(log_file, "a", stderr))) {
			fclose(stdout);
			stdout = orig_out;
			stderr = orig_err;
			log_critical("Unable to rotate the error log. { file = %s }", log_file);
			pthread_mutex_unlock(&log_mutex);
			return;
		}
		pthread_mutex_unlock(&log_mutex);
	}

}

bool_t log_start(void) {

	FILE *file_out, *file_err;
	chr_t *log_file = MEMORYBUF(1024);

	// File logging.
	if (magma.output.file && magma.output.path) {

		log_date = time_datestamp();

		if (snprintf(log_file, 1024, "%s%smagmad.%lu.log", magma.output.path, (*(ns_length_get(magma.output.path) + magma.output.path) == '/') ? "" : "/",
		             log_date) >= 1024) {
			log_critical("Log file path exceeded available buffer. { file = %s%smagmad.%lu.log }", magma.output.path,
			             (*(ns_length_get(magma.output.path) + magma.output.path) == '/') ? "" : "/", log_date);
			return false;
		}

		if (folder_exists(NULLER(magma.output.path), false)) {
			log_critical("The path configured to hold the output log files does not exist. { path = %s }", magma.output.path);
			return false;
		}

		if (!(file_out = freopen64(log_file, "a+", stdout))) {
			log_critical("Unable to open the error log, sticking with standard out. { file = %s }", log_file);
			return false;
		}

		if (!(file_err = freopen64(log_file, "a+", stderr))) {
			log_critical("Unable to open the error log, sticking with standard error. { file = %s }", log_file);
			fclose(file_out);
			return false;
		}

		stdout = file_out;
		stderr = file_err;
	}

	fclose(stdin);
	return true;
}

/**
 * @brief	A stub routine where it is convenient to set a breakpoint in a debugger.
 * @note	This function should be called by code paths that detect sanity check failures, but that are not fatal.
 */
void debug_hook(void) {

	log_pedantic("Triggered debug hook.");
}
