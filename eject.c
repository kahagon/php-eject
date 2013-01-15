/*
   +----------------------------------------------------------------------+
   | This source file is subject to version 3.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available through the world-wide-web at the following url:           |
   | http://www.php.net/license/3_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Keng-ichi Ahagon <k.ahagon@n-3.so>                          |
   +----------------------------------------------------------------------+
*/

/* $ Id: $ */ 

#include "php_eject.h"

#if HAVE_EJECT

/* {{{ eject_functions[] */
function_entry eject_functions[] = {
	PHP_FE(eject_close_tray    , eject_close_tray_arg_info)
	PHP_FE(eject_toggle_tray   , eject_toggle_tray_arg_info)
	{ NULL, NULL, NULL }
};
/* }}} */


/* {{{ eject_module_entry
 */
zend_module_entry eject_module_entry = {
	STANDARD_MODULE_HEADER,
	"eject",
	eject_functions,
	PHP_MINIT(eject),     /* Replace with NULL if there is nothing to do at php startup   */ 
	PHP_MSHUTDOWN(eject), /* Replace with NULL if there is nothing to do at php shutdown  */
	PHP_RINIT(eject),     /* Replace with NULL if there is nothing to do at request start */
	PHP_RSHUTDOWN(eject), /* Replace with NULL if there is nothing to do at request end   */
	PHP_MINFO(eject),
	PHP_EJECT_VERSION, 
	STANDARD_MODULE_PROPERTIES
};
/* }}} */

#ifdef COMPILE_DL_EJECT
ZEND_GET_MODULE(eject)
#endif


/* {{{ PHP_MINIT_FUNCTION */
PHP_MINIT_FUNCTION(eject)
{

	/* add your stuff here */

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MSHUTDOWN_FUNCTION */
PHP_MSHUTDOWN_FUNCTION(eject)
{

	/* add your stuff here */

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_RINIT_FUNCTION */
PHP_RINIT_FUNCTION(eject)
{
	/* add your stuff here */

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_RSHUTDOWN_FUNCTION */
PHP_RSHUTDOWN_FUNCTION(eject)
{
	/* add your stuff here */

	return SUCCESS;
}
/* }}} */


/* {{{ PHP_MINFO_FUNCTION */
PHP_MINFO_FUNCTION(eject)
{
	php_printf("eject binding for PHP\n");
	php_info_print_table_start();
	php_info_print_table_row(2, "Version",PHP_EJECT_VERSION " (alpha)");
	php_info_print_table_row(2, "Released", "2012-10-08");
	php_info_print_table_row(2, "CVS Revision", "$Id: $");
	php_info_print_table_row(2, "Authors", "Keng-ichi Ahagon 'k.ahagon@n-3.so' (lead)\n");
	php_info_print_table_end();
	/* add your stuff here */

}
/* }}} */

#define FCLOSE(fd) if (fclose(fd)==-1) { \
    php_error(E_ERROR, "Unable to close FILE pointer."); \
}

/* Used by the ToggleTray() function. If ejecting the tray takes this
 * time or less, the tray was probably already ejected, so we close it
 * again.
 */
#define TRAY_WAS_ALREADY_OPEN_USECS  200000	/* about 0.2 seconds */

#define EJECT_COMMAND_CLOSE 0
#define EJECT_COMMAND_TOGGLE 1

#ifdef PHP_WIN32

#else

/* Return 1 if file/device exists, 0 otherwise. */
static int FileExists(const char *name)
{

	/*
	 * access() uses the UID, not the EUID. This way a normal user
	 * cannot find out if a file (say, /root/fubar) exists or not, even
	 * if eject is SUID root
	 */
	if (access (name, F_OK) == 0) {
		return 1;
	}	else {
		return 0;
	}
}

/*
 * Given name, such as foo, see if any of the following exist:
 *
 * foo (if foo starts with '.' or '/')
 * /dev/foo
 * /media/foo
 * /mnt/foo
 * /dev/cdroms/foo
 * /dev/cdroms/foo0
 * /dev/dsk/foo
 * /dev/rdsk/foo
 * ./foo
 *
 * If found, return the full path. If not found, return 0.
 * Returns pointer to dynamically allocated string.
 */
static char *FindDevice(const char *name)
{
	char *buf = NULL;

	buf = (char *) emalloc(strlen(name)+14); /* to allow for "/dev/cdroms/ + "0" + null */
	if (buf==NULL) {
		php_error(E_WARNING, "could not allocate memory\n");
		return NULL;
	}
	if ((name[0] == '.') || (name[0] == '/')) {
		strcpy(buf, name);
		if (FileExists(buf))
			return buf;
	}

	strcpy(buf, "/dev/");
	strcat(buf, name);
	if (FileExists(buf))
		return buf;

	strcpy(buf, "/media/");
	strcat(buf, name);
	if (FileExists(buf))
		return buf;
	
	strcpy(buf, "/mnt/");
	strcat(buf, name);
	if (FileExists(buf))
		return buf;

	/* for devfs under Linux */
	strcpy(buf, "/dev/cdroms/");
	strcat(buf, name);
	if (FileExists(buf))
		return buf;

	strcpy(buf, "/dev/cdroms/");
	strcat(buf, name);
	strcat(buf, "0");
	if (FileExists(buf))
		return buf;

	/* for devfs under Solaris */
	strcpy(buf, "/dev/rdsk/");
	strcat(buf, name);
	if (FileExists(buf))
		return buf;

	strcpy(buf, "/dev/dsk/");
	strcat(buf, name);
	if (FileExists(buf))
		return buf;

	strcpy(buf, "./");
	strcat(buf, name);
	if (FileExists(buf))
		return buf;

	efree(buf);
	return NULL;
}


/* Check if name is a symbolic link. If so, return what it points to. */
static char *SymLink(const char *name)
{
	int status;
	char s1[PATH_MAX];
	char s2[PATH_MAX];
	char s4[PATH_MAX];
	char result[PATH_MAX];
	char *s3;

	memset(s1, 0, sizeof(s1));
	memset(s2, 0, sizeof(s2));
	memset(s4, 0, sizeof(s4));
	memset(result, 0, sizeof(result));

	status = readlink(name, s1, sizeof(s1) - 1);

	if (status == -1)
		return 0;

	s1[status] = 0;
	if (s1[0] == '/') { /* absolute link */
		return estrdup(s1);
	} else { /* relative link, add base name */
		strncpy(s2, name, sizeof(s2)-1);
		s3 = strrchr(s2, '/');
		if (s3 != 0) {
			s3[1] = 0;
			snprintf(result, sizeof(result)-1, "%s%s", s2, s1);
		}
	}
	char *c = realpath(result, s4);
        if (!c) {
            return NULL;
        } else {
            return estrdup(s4);
        }
}

/*
 * Get major and minor device numbers for a device file name, so we
 * can check for duplicate devices.
 */
static int GetMajorMinor(const char *name, int *maj, int *min)
{
	struct stat sstat;
	*maj = *min = -1;
	if (stat(name, &sstat) == -1)
		return -1;
	if (! S_ISBLK(sstat.st_mode))
		return -1;
	*maj = major(sstat.st_rdev);
	*min = minor(sstat.st_rdev);
	return 0;
}

/*
 * See if device has been mounted by looking in mount table.  If so, set
 * device name and mount point name, and return 1, otherwise return 0.
 */
static int MountedDevice(const char *name, char **mountName, char **deviceName, int p_option)
{
	FILE *fp;
	char line[1024];
	char s1[1024];
	char s2[1024];
	int rc;

	int maj;
	int min;

	GetMajorMinor(name, &maj, &min);

	fp = fopen((p_option ? "/proc/mounts" : "/etc/mtab"), "r");
	if (fp == NULL)
	{
            php_error(E_WARNING, "unable to open %s: %s\n", (p_option ? "/proc/mounts" : "/etc/mtab"), strerror(errno));
            return 0;
	}

	while (fgets(line, sizeof(line), fp) != 0) {
		rc = sscanf(line, "%1023s %1023s", s1, s2);
		if (rc >= 2) {
			int mtabmaj, mtabmin;
			GetMajorMinor(s1, &mtabmaj, &mtabmin);
			if (((strcmp(s1, name) == 0) || (strcmp(s2, name) == 0)) ||
				((maj != -1) && (maj == mtabmaj) && (min == mtabmin))) {
				FCLOSE(fp);
				*deviceName = estrdup(s1);
				*mountName = estrdup(s2);
				return 1;
			}
		}
	}
	*deviceName = 0;
	*mountName = 0;
	FCLOSE(fp);
	return 0;
}

/*
 * See if device can be mounted by looking in /etc/fstab.
 * If so, set device name and mount point name, and return 1,
 * otherwise return 0.
 */
static int MountableDevice(const char *name, char **mountName, char **deviceName)
{
	FILE *fp;
	char line[1024];
	char s1[1024];
	char s2[1024];
	int rc;

	fp = fopen("/etc/fstab", "r");
	if (fp == NULL) {
/*
 * /etc/fstab may be unreadable in some situations due to passwords in the
 * file.
 */
		return -1;
	}

	while (fgets(line, sizeof(line), fp) != 0) {
		rc = sscanf(line, "%1023s %1023s", s1, s2);
		if (rc >= 2 && s1[0] != '#' && strcmp(s2, name) == 0) {
			FCLOSE(fp);
			*deviceName = estrdup(s1);
			*mountName = estrdup(s2);
			return 1;
		}
	}
	FCLOSE(fp);
	return 0;
}

/* Open a device file. */
static int OpenDevice(const char *fullName)
{
	int fd = open(fullName, O_RDONLY|O_NONBLOCK);
	return fd;
}

/*
 * Close tray. Not supported by older kernels.
 */
static int CloseTray(int fd)
{
	int status;

#ifdef CDROMCLOSETRAY
	status = ioctl(fd, CDROMCLOSETRAY);
	if (status != 0) {
            php_error(E_WARNING, "CD-ROM tray close command failed: %s\n", strerror(errno));
            return 0;
	} else {
            return 1;
        }
#else
        php_error(E_WARNING, "CD-ROM tray close command not supported by this kernel\n");
        return 0;
#endif
}


/*
 * Toggle tray.
 *
 * Written by Benjamin Schwenk <benjaminschwenk@yahoo.de> and
 * Sybren Stuvel <sybren@thirdtower.com>
 *
 * Not supported by older kernels because it might use
 * CloseTray().
 *
 */
static int ToggleTray(int fd)
{
	struct timeval time_start, time_stop;
	int time_elapsed;

#ifdef CDROMCLOSETRAY

	/* Try to open the CDROM tray and measure the time therefor
	 * needed.  In my experience the function needs less than 0.05
	 * seconds if the tray was already open, and at least 1.5 seconds
	 * if it was closed.  */
	gettimeofday(&time_start, NULL);
	
	/* Send the CDROMEJECT command to the device. */
	if (ioctl(fd, CDROMEJECT, 0) < 0) {
		perror("ioctl");
		return 0;
	}

	/* Get the second timestamp, to measure the time needed to open
	 * the tray.  */
	gettimeofday(&time_stop, NULL);

	time_elapsed = (time_stop.tv_sec * 1000000 + time_stop.tv_usec) -
		(time_start.tv_sec * 1000000 + time_start.tv_usec);

	/* If the tray "opened" too fast, we can be nearly sure, that it
	 * was already open. In this case, close it now. Else the tray was
	 * closed before. This would mean that we are done.  */
	if (time_elapsed < TRAY_WAS_ALREADY_OPEN_USECS) {
            return CloseTray(fd);
        } else {
            return 1;
        }
		

#else
    return 0;
    php_error(E_WARNING, "CD-ROM tray toggle command not supported by this kernel\n");
#endif
	
}
#endif /* PHP_WIN32 */

static int eject_impl(const char *_device, int device_len, int command, zend_bool use_proc_mount) {
    int status = 0;
#ifdef PHP_WIN32
    php_error(E_WARNING, "not implemented yet\n");
    return 0;
#else
    char *fullName;    /* expanded name */
    int ld = 6;	   /* symbolic link max depth */
    char *linkName;    /* name of device's symbolic link */
    int mounted = 0;   /* true if device is mounted */
    char *mountName;   /* name of device's mount point */
    char *deviceName;  /* name of device */
    int mountable = 0; /* true if device is in /etc/fstab */
    int fd; 	   /* file descriptor for device */\
    
    char *device = estrdup(_device);
    if (!device) {
        php_error(E_ERROR, "failed to allocate memory.\n");
        return 0;
    }
    
    /* Strip any trailing slash from name in case user used bash/tcsh
       style filename completion (e.g. /mnt/cdrom/) */
    if (device[device_len - 1] == '/')
            device[device_len - 1] = 0;
    
    /* figure out full device or mount point name */
    fullName = FindDevice(device);
    if (!fullName) {
            php_error(E_WARNING, "unable to find or open device for: `%s'\n", device);
            return 0;
    }
    
    /* check for a symbolic link */
    while ((linkName = SymLink(fullName)) && (ld > 0)) {
            efree(fullName);
            fullName = strdup(linkName);
            efree(linkName);
            linkName = 0;
            ld--;
    }
    /* handle max depth exceeded option */
    if (ld <= 0) {
            php_error(E_WARNING, "maximum symbolic link depth exceeded: `%s'\n", fullName);
            return 0;
    }
    
    /* if mount point, get device name */
    mounted = MountedDevice(fullName, &mountName, &deviceName, (int)use_proc_mount);
    if (!mounted) {
            deviceName = estrdup(fullName);
    }
    
    /* if not currently mounted, see if it is a possible mount point */
    if (!mounted) {
            mountable = MountableDevice(fullName, &mountName, &deviceName);
    }
    
    switch (command) {
        case EJECT_COMMAND_CLOSE:
            fd = OpenDevice(deviceName);
            if (fd == -1) {
                php_error(E_WARNING, "unable to open `%s'\n", fullName);
                return 0;
            } else {
                status = CloseTray(fd);
            }
            break;
        case EJECT_COMMAND_TOGGLE:
            fd = OpenDevice(deviceName);
            if (fd == -1) {
                php_error(E_WARNING, "unable to open `%s'\n", fullName);
                status = 0;
            } else {
                status = ToggleTray(fd);
            }
            break;
        default:
            status = 0;
            break;
    }
    
    if (mountName) efree(mountName);
    if (deviceName) efree(deviceName);
    return status;
#endif /* PHP_WIN32 */
}

/* {{{ proto bool eject_close_tray(string device [, bool use_proc_mount])
   */
PHP_FUNCTION(eject_close_tray)
{
	const char * device = NULL;
	int device_len = 0;
	zend_bool use_proc_mount = 0;
        
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b", &device, &device_len, &use_proc_mount) == FAILURE) {
		return;
	}
        
        if (eject_impl(device, device_len, EJECT_COMMAND_CLOSE, use_proc_mount)) {
            RETURN_TRUE;
        } else {
            RETURN_FALSE;
        }
}
/* }}} eject_close_tray */


/* {{{ proto bool eject_toggle_tray(string device [, bool use_proc_mount])
   */
PHP_FUNCTION(eject_toggle_tray)
{
	const char * device = NULL;
	int device_len = 0;
	zend_bool use_proc_mount = 0;
        
	if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s|b", &device, &device_len, &use_proc_mount) == FAILURE) {
		return;
	}

        if (eject_impl(device, device_len, EJECT_COMMAND_TOGGLE, use_proc_mount)) {
            RETURN_TRUE;
        } else {
            RETURN_FALSE;
        }
}
/* }}} eject_toggle_tray */

#endif /* HAVE_EJECT */


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
