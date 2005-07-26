#ifndef lint
static const char rcsid[] =
  "$FreeBSD: src/sbin/shdconfig/shdconfig.c,v 1.16.2.2 2000/12/11 01:03:25 obrien Exp $";
#endif /* not lint */

#include <sys/param.h>
#include <sys/linker.h>
#include <sys/disklabel.h>
#include <sys/stat.h>
#include <sys/module.h>
#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/devicestat.h>
#include <sys/shdvar.h>

#include "pathnames.h"
#include "trie.h"

extern struct proc *curproc;

#define MAXBUF 262144 
#define BLOCKSIZE 512

static	int lineno = 0;
static	int verbose = 0;
static	char *shdconf = _PATH_SHDCONF;

static	char *core = NULL;
static	char *kernel = NULL;

struct	flagval {
	char	*fv_flag;
	int	fv_val;
} flagvaltab[] = {
	{ "SHDF_ONETOONE",	SHDF_ONETOONE },
	{ "SHDF_COMPACT",	SHDF_COMPACT },
	{ "SHDF_COR",		SHDF_COR },
	{ "SHDF_COW",		SHDF_COW },
	{ NULL,			0 },
};

static	struct nlist nl[] = {
	{ "_shd_softc" },
#define SYM_SHDSOFTC		0
	{ "_numshd" },
#define SYM_NUMSHD		1
	{ NULL },
};

#define SHD_CONFIG		0	/* configure a device */
#define SHD_CONFIGALL		1	/* configure all devices */
#define SHD_UNCONFIG		2	/* unconfigure a device */
#define SHD_UNCONFIGALL		3	/* unconfigure all devices */
#define SHD_DUMP		4	/* dump a shd's configuration */

static	int checkdev __P((char *));
static	int do_io __P((char *, u_long, struct shd_ioctl *));
static	int do_single __P((int, char **, int, int));
static	int do_all __P((int));
static	int dump_shd __P((int, char **));
static	int getmaxpartitions __P((void));
static	int getrawpartition __P((void));
static	int flags_to_val __P((char *));
static	void print_shd_info __P((struct shd_softc *, kvm_t *));
static	char *resolve_shdname __P((char *));
static	void usage __P((void));
int save_checkpoint (char * shd, int version);
int load_checkpoint (int version);

int
main(argc, argv)
	int argc;
	char **argv;
{
	int ch, options = 0, action = SHD_CONFIG;
	int flags = 0;
        int version, maxversion;
        int old_ver;

        if (strcmp (argv[1], "-l") == 0)
        {
            version = atoi(argv[2]); 
            maxversion = atoi(argv[3]); 
            printf ("Loading version %d\n", version);
            for (old_ver = maxversion; old_ver >= version; old_ver-- )
            {
                load_checkpoint (old_ver);
            }
            return (0);
        }
	if (modfind("shd") < 0) {
		/* Not present in kernel, try loading it */
		if (kldload("shd") < 0 || modfind("shd") < 0)
			warn("shd module not available!");
	}

			exit(do_single(argc, argv, action, flags));
	return (0);
}

static int
do_single(argc, argv, action, flags)
	int argc;
	char **argv;
	int action;
	int flags;
{
	struct shd_ioctl shio;
	char *shd, *cp, *cp2, *srcdisk, *copydisk;
	int noflags = 0, i, ileave, j;
        int version, maxversion;
        int old_ver;


	bzero(&shio, sizeof(shio));

        *argv++; --argc;
	/* First argument is the shd to configure. */
	cp = *argv++; --argc;
	if ((shd = resolve_shdname(cp)) == NULL) {
		warnx("invalid shd name: %s", cp);
		return (1);
	}

	/* Next argument is the source disk */
	cp = *argv++; --argc;
	if ((j = checkdev(cp)) == 0)
		srcdisk = cp;
	else {
		warnx("%s: %s", cp, strerror(j));
		return (1);
	}

	/* Next argument is the 1st copy disk */
	cp = *argv++; --argc;
	if ((j = checkdev(cp)) == 0)
		copydisk = cp;
	else {
		warnx("%s: %s", cp, strerror(j));
		return (1);
	}

	/* Fill in the shio. */
	shio.shio_srcdisk = srcdisk;
	shio.shio_copydisk = copydisk;
	shio.shio_flags = flags;

        cp = *argv++; --argc;
        shio.shio_flags |= SHDF_INITIALIZE;

        if (strcmp (cp, "-i") == 0)
        {
	    if (do_io(shd, SHDIOCSET, &shio))
		return (1);
        }
        else
        if (strcmp (cp, "-c") == 0)
        {
            time_t rawtime;
            struct tm * timeinfo;
            struct shd_ckpt ckpt;
            time ( &rawtime );
            timeinfo = localtime ( &rawtime );
            if (*argv)
                ckpt.name = *argv++; --argc;
            ckpt.time = asctime (timeinfo);        
            printf ("Name = %s\n", ckpt.name);
            printf ("Time = %s\n", ckpt.time);
            if (do_io(shd, SHDCHECKPOINT, &ckpt))
            {
                return (1);
            }
        }
        else
        if (strcmp (cp, "-g") == 0)
        {
            if (do_io(shd, SHDGETCHECKPOINTS, &shio))
                return (1);
        }
        else
        if (strcmp (cp, "-e") == 0)
        {
            if (do_io(shd, SHDENABLECHECKPOINTING, &shio))
                return (1);
        }
        else
        if (strcmp (cp, "-d") == 0)
        {
            if (do_io(shd, SHDDISABLECHECKPOINTING, &shio))
                return (1);
        }
        else
        if (strcmp (cp, "-delete") == 0)
        {       
            shio.delete_start = atoi(*argv++); --argc;
            shio.delete_end = atoi(*argv++); --argc;
            if (do_io(shd, SHDDELETECHECKPOINTS, &shio))
                return (1);
        }
        else
        if (strcmp (cp, "-crash") == 0)
        {
            if (do_io(shd, SHDCRASH, &shio))
                return (1);
        }
        else
        if (strcmp (cp, "-r") == 0)
        {       
            version = atoi(*argv++); --argc;
            printf ("Rolling back to version %d\n", version);
            shio.version = version;
            if (do_io(shd, SHDROLLBACK, &shio))
                return (1);
        }
        else
        if (strcmp (cp, "-rv") == 0)
        {   
            version = atoi(*argv++); --argc;
            printf ("Reboot version set to %d\n", version);
            shio.version = version;
            if (do_io(shd, SHDSETREBOOTVERSION, &shio))
                return (1);
        }
        else
        if (strcmp (cp, "-sm") == 0)
        {
            if (do_io(shd, SHDSAVECHECKPOINTMAP, &shio))
                return (1);
        }
        else    
        if (strcmp (cp, "-sv") == 0)
        {       
            version = atoi(*argv++); --argc;
            maxversion = atoi(*argv++); --argc;
            for (old_ver = version; old_ver <= maxversion; old_ver++)
            {
                printf ("Saving version %d\n", old_ver);
                save_checkpoint (shd, old_ver);
            }
        }               
        else
        if (strcmp (cp, "-l") == 0)
        {
            version = atoi(*argv++); --argc;
            maxversion = atoi(*argv++); --argc;
            printf ("Loading version %d\n", version);
            for (old_ver = maxversion; old_ver >= version; old_ver-- )
            {
                load_checkpoint (old_ver);
            }
        }   
        else
        if (strcmp (cp, "-gm") == 0)
        {
            struct shd_range {
               u_int32_t start;
               u_int32_t end;
            };

            struct shd_modinfo sm;
            sm.bufsiz = 64;
            sm.buf = malloc(sm.bufsiz * sizeof(struct shd_range));
            sm.command = 1;
            if (do_io(shd, SHDGETMODIFIEDRANGES, &sm))
                return (1); 
            while (sm.retsiz > 0) {
                struct shd_range *sr = sm.buf;
               
                for (sr = sm.buf; sm.retsiz > 0; sr++, sm.retsiz--) {
                    printf ("%ld<->%ld\n", sr->start, sr->end);
                }
                sm.command = 2;
                if (do_io(shd, SHDGETMODIFIEDRANGES, &sm) < 0) {
                        /* XXX should flush the valid table */
                        return 1;
                }
            } 
            sm.command = 3;
            do_io(shd, SHDGETMODIFIEDRANGES, &sm);
            free (sm.buf);
 
            /* 
            long buf[512];
            struct shd_modinfo mod;
            mod.command = 1; 
            mod.buf = buf;
            mod.bufsiz = 512;

            for (i = 0; i < 512; i++)
                    buf[i] = 0;
            mod.retsiz = 0;
            if (do_io(shd, SHDGETMODIFIEDRANGES, &mod))
                return (1);
            printf ("retsize = %d\n", mod.retsiz);
            if (mod.retsiz == -1)
            {
                printf ("Error getting modified blocks\n");
                return (1);
            }
            for (i = 0; i < mod.retsiz; i++)
            {
                printf ("%ld <-> %ld\n", buf[i*2], buf[i*2+1]);
            }
            while (mod.retsiz > 0)
            {
                for (i = 0; i < 512; i++)
                    buf[i] = 0;
                mod.retsiz = 0;
                mod.command = 2;
                if (do_io(shd, SHDGETMODIFIEDRANGES, &mod))
                    return (1);
                printf ("retsize = %d\n", mod.retsiz);
                if (mod.retsiz == -1)
                {
                    printf ("Error getting modified blocks\n");
                    return (1);
                }
                for (i = 0; i < mod.retsiz; i++)
                {
                    printf ("%ld <-> %ld\n", buf[i*2], buf[i*2+1]);
                }
            } 
            for (i = 0; i < 512; i++)
                buf[i] = 0;
            mod.retsiz = 0;
            mod.command = 3; 
            if (do_io(shd, SHDGETMODIFIEDRANGES, &mod))
                return (1);*/
        }

	/*printf("shd%d: ", shio.shio_unit);
	printf("%lu blocks ", (u_long)shio.shio_size);*/

	return (0);
}

static int
do_all(action)
	int action;
{
	FILE *f;
	char line[_POSIX2_LINE_MAX];
	char *cp, **argv;
	int argc, rval;
	gid_t egid;

	rval = 0;
	egid = getegid();
	setegid(getgid());
	if ((f = fopen(shdconf, "r")) == NULL) {
		setegid(egid);
		warn("fopen: %s", shdconf);
		return (1);
	}
	setegid(egid);

	while (fgets(line, sizeof(line), f) != NULL) {
		argc = 0;
		argv = NULL;
		++lineno;
		if ((cp = strrchr(line, '\n')) != NULL)
			*cp = '\0';

		/* Break up the line and pass it's contents to do_single(). */
		if (line[0] == '\0')
			goto end_of_line;
		for (cp = line; (cp = strtok(cp, " \t")) != NULL; cp = NULL) {
			if (*cp == '#')
				break;
			if ((argv = realloc(argv,
			    sizeof(char *) * ++argc)) == NULL) {
				warnx("no memory to configure shds");
				return (1);
			}
			argv[argc - 1] = cp;
			/*
			 * If our action is to unconfigure all, then pass
			 * just the first token to do_single() and ignore
			 * the rest.  Since this will be encountered on
			 * our first pass through the line, the Right
			 * Thing will happen.
			 */
			if (action == SHD_UNCONFIGALL) {
				if (do_single(argc, argv, action, 0))
					rval = 1;
				goto end_of_line;
			}
		}
		if (argc != 0)
			if (do_single(argc, argv, action, 0))
				rval = 1;

 end_of_line:
		if (argv != NULL)
			free(argv);
	}

	(void)fclose(f);
	return (rval);
}

static int
checkdev(path)
	char *path;
{
	struct stat st;

	if (stat(path, &st) != 0)
		return (errno);

	if (!S_ISBLK(st.st_mode) && !S_ISCHR(st.st_mode) &&
	    !S_ISREG(st.st_mode))
		return (EINVAL);

	return (0);
}

static int
pathtounit(path, unitp)
	char *path;
	int *unitp;
{
	struct stat st;
	int maxpartitions;

	if (stat(path, &st) != 0)
		return (errno);

	if (!S_ISBLK(st.st_mode) && !S_ISCHR(st.st_mode))
		return (EINVAL);

	if ((maxpartitions = getmaxpartitions()) < 0)
		return (errno);

	*unitp = minor(st.st_rdev) / maxpartitions;

	return (0);
}

static char *
resolve_shdname(name)
	char *name;
{
	char c, *path;
	size_t len, newlen;
	int rawpart;

	if (name[0] == '/' || name[0] == '.') {
		/* Assume they gave the correct pathname. */
		return (strdup(name));
	}

	len = strlen(name);
	c = name[len - 1];

	newlen = len + 8;
	if ((path = malloc(newlen)) == NULL)
		return (NULL);
	bzero(path, newlen);

	if (isdigit(c)) {
		if ((rawpart = getrawpartition()) < 0) {
			free(path);
			return (NULL);
		}
		(void)sprintf(path, "%s%s%c", _PATH_DEV, name, 'a' + rawpart);
	} else
		(void)sprintf(path, "%s%s", _PATH_DEV, name);

	return (path);
}

static int
do_io(path, cmd, shiop)
	char *path;
	u_long cmd;
	struct shd_ioctl *shiop;
{
	int fd;
	char *cp;

	if ((fd = open(path, O_RDWR, 0640)) < 0) {
		warn("open: %s", path);
		return (1);
	}

	if (ioctl(fd, cmd, shiop) < 0) {
		switch (cmd) {
                case SHDCHECKPOINT:
                        cp = "SHDCHECKPOINT";
                        break;

		case SHDIOCSET:
			cp = "SHDIOCSET";
			break;

		case SHDIOCCLR:
			cp = "SHDIOCCLR";
			break;

		default:
			cp = "unknown";
		}
		warn("ioctl (%s): %s", cp, path);
		return (1);
	}

	return (0);
}

#define KVM_ABORT(kd, str) {						\
	(void)kvm_close((kd));						\
	warnx("%s", (str));							\
	warnx("%s", kvm_geterr((kd)));					\
	return (1);							\
}

static int
dump_shd(argc, argv)
	int argc;
	char **argv;
{
	char errbuf[_POSIX2_LINE_MAX], *shd, *cp;
	struct shd_softc *cs, *kcs;
	size_t readsize;
	int i, error, numshd, numconfiged = 0;
	kvm_t *kd;

	bzero(errbuf, sizeof(errbuf));

	if ((kd = kvm_openfiles(kernel, core, NULL, O_RDONLY,
	    errbuf)) == NULL) {
		warnx("can't open kvm: %s", errbuf);
		return (1);
	}

	if (kvm_nlist(kd, nl))
		KVM_ABORT(kd, "shd-related symbols not available");

	/* Check to see how many shds are currently configured. */
	if (kvm_read(kd, nl[SYM_NUMSHD].n_value, (char *)&numshd,
	    sizeof(numshd)) != sizeof(numshd))
		KVM_ABORT(kd, "can't determine number of configured shds");

	if (numshd == 0) {
		printf("shd driver in kernel, but is uninitialized\n");
		goto done;
	}

	/* Allocate space for the configuration data. */
	readsize = numshd * sizeof(struct shd_softc);
	if ((cs = malloc(readsize)) == NULL) {
		warnx("no memory for configuration data");
		goto bad;
	}
	bzero(cs, readsize);

	/*
	 * Read the shd configuration data from the kernel and dump
	 * it to stdout.
	 */
	if (kvm_read(kd, nl[SYM_SHDSOFTC].n_value, (char *)&kcs,
	    sizeof(kcs)) != sizeof(kcs)) {
		free(cs);
		KVM_ABORT(kd, "can't find pointer to configuration data");
	}
	if (kvm_read(kd, (u_long)kcs, (char *)cs, readsize) != readsize) {
		free(cs);
		KVM_ABORT(kd, "can't read configuration data");
	}

	if (argc == 0) {
		for (i = 0; i < numshd; ++i)
			if (cs[i].sc_flags & SHDF_INITED) {
				++numconfiged;
				print_shd_info(&cs[i], kd);
			}

		if (numconfiged == 0)
			printf("no concatenated disks configured\n");
	} else {
		while (argc) {
			cp = *argv++; --argc;
			if ((shd = resolve_shdname(cp)) == NULL) {
				warnx("invalid shd name: %s", cp);
				continue;
			}
			if ((error = pathtounit(shd, &i)) != 0) {
				warnx("%s: %s", shd, strerror(error));
				continue;
			}
			if (i >= numshd) {
				warnx("shd%d not configured", i);
				continue;
			}
			if (cs[i].sc_flags & SHDF_INITED)
				print_shd_info(&cs[i], kd);
			else
				printf("shd%d not configured\n", i);
		}
	}

	free(cs);

 done:
	(void)kvm_close(kd);
	return (0);

 bad:
	(void)kvm_close(kd);
	return (1);
}

static void
print_shd_info(cs, kd)
	struct shd_softc *cs;
	kvm_t *kd;
{
#if 0
	static int header_printed = 0;
	struct shdcinfo *cip;
	size_t readsize;
	char path[MAXPATHLEN];
	int i;

	if (header_printed == 0 && verbose) {
		printf("# shd\t\tileave\tflags\tcompnent devices\n");
		header_printed = 1;
	}

	readsize = cs->sc_nshdisks * sizeof(struct shdcinfo);
	if ((cip = malloc(readsize)) == NULL) {
		warn("shd%d: can't allocate memory for component info",
		    cs->sc_unit);
		return;
	}
	bzero(cip, readsize);

	/* Dump out softc information. */
	printf("shd%d\t\t%d\t%d\t", cs->sc_unit, cs->sc_ileave,
	    cs->sc_cflags & SHDF_USERMASK);
	fflush(stdout);

	/* Read in the component info. */
	if (kvm_read(kd, (u_long)cs->sc_cinfo, (char *)cip,
	    readsize) != readsize) {
		printf("\n");
		warnx("can't read component info");
		warnx("%s", kvm_geterr(kd));
		goto done;
	}

	/* Read component pathname and display component info. */
	for (i = 0; i < cs->sc_nshdisks; ++i) {
		if (kvm_read(kd, (u_long)cip[i].ci_path, (char *)path,
		    cip[i].ci_pathlen) != cip[i].ci_pathlen) {
			printf("\n");
			warnx("can't read component pathname");
			warnx("%s", kvm_geterr(kd));
			goto done;
		}
		printf((i + 1 < cs->sc_nshdisks) ? "%s " : "%s\n", path);
		fflush(stdout);
	}

 done:
	free(cip);
#endif
}

static int
getmaxpartitions()
{
    return (MAXPARTITIONS);
}

static int
getrawpartition()
{
	return (RAW_PART);
}

static int
flags_to_val(flags)
	char *flags;
{
	char *cp, *tok;
	int i, tmp, val = ~SHDF_USERMASK;
	size_t flagslen;

	/*
	 * The most common case is that of NIL flags, so check for
	 * those first.
	 */
	if (strcmp("none", flags) == 0 || strcmp("0x0", flags) == 0 ||
	    strcmp("0", flags) == 0)
		return (0);

	flagslen = strlen(flags);

	/* Check for values represented by strings. */
	if ((cp = strdup(flags)) == NULL)
		err(1, "no memory to parse flags");
	tmp = 0;
	for (tok = cp; (tok = strtok(tok, ",")) != NULL; tok = NULL) {
		for (i = 0; flagvaltab[i].fv_flag != NULL; ++i)
			if (strcmp(tok, flagvaltab[i].fv_flag) == 0)
				break;
		if (flagvaltab[i].fv_flag == NULL) {
			free(cp);
			goto bad_string;
		}
		tmp |= flagvaltab[i].fv_val;
	}

	/* If we get here, the string was ok. */
	free(cp);
	val = tmp;
	goto out;

 bad_string:

	/* Check for values represented in hex. */
	if (flagslen > 2 && flags[0] == '0' && flags[1] == 'x') {
		errno = 0;	/* to check for ERANGE */
		val = (int)strtol(&flags[2], &cp, 16);
		if ((errno == ERANGE) || (*cp != '\0'))
			return (-1);
		goto out;
	}

	/* Check for values represented in decimal. */
	errno = 0;	/* to check for ERANGE */
	val = (int)strtol(flags, &cp, 10);
	if ((errno == ERANGE) || (*cp != '\0'))
		return (-1);

 out:
	return (((val & ~SHDF_USERMASK) == 0) ? val : -1);
}

static void
usage()
{
	fprintf(stderr, "%s\n%s\n%s\n%s\n%s\n",
		"usage: shdconfig [-cv] shd [flags] srcdev [size] copydev [size]",
		"       shdconfig -C [-v] [-f config_file]",
		"       shdconfig -u [-v] shd [...]",
		"       shdconfig -U [-v] [-f config_file]",
		"       shdconfig -g [-M core] [-N system] [shd [...]]");
	exit(1);
}

char *
itoa(int value)
{
    static char buf[13];

    snprintf(buf, 12, "%d", value);
    return buf;
}


int save_checkpoint (char * shd, int version)
{
    struct shd_readbuf shread;
    char buffer [MAXBUF];
    long block [BLOCKSIZE/4 + 1];
    off_t off;
    int i, j;
    int ix;
    int fd_read;
    int fd_write_data;
    int fd_write_mdata;
    int size;
    int read_start;
    int num_blocks;
    char path[256];
    char ver_buf[2]; 
    char *ver = &ver_buf; 
    ver = itoa (version);
    strcpy (path, "/proj/tbres/images/image");
    strcat (path, ver);
    strcat (path, ".data");
    
    fd_read = open ("/dev/ad0s4", O_RDWR);
    if (fd_read < 0)
    {
        printf ("Error opening /dev/ad0s4\n");
        perror ("error");
        return;
    }

    /* Open the data file to write to */
    
    fd_write_data = open (path, O_CREAT);
    if (fd_write_data < 0)
    {
        printf ("Error opening %s O_CREAT\n", path);
        perror ("error");
        return;
    }
    close (fd_write_data);
    fd_write_data = open (path, O_WRONLY);
    if (fd_write_data < 0)
    {
        printf ("Error opening %s O_WRONLY\n", path);
        perror ("error");
        return;
    }

    /* Open the metadata file to write to */

    strcpy (path, "/proj/tbres/images/image");
    strcat (path, ver);
    strcat (path, ".mdata");

    fd_write_mdata = open (path, O_CREAT);
    if (fd_write_mdata < 0)
    {
        printf ("Error opening %s O_CREAT\n", path);
        perror ("error");
        return;
    }
    close (fd_write_mdata);
    fd_write_mdata = open (path, O_WRONLY);
    if (fd_write_mdata < 0)
    {
        printf ("Error opening %s O_WRONLY\n", path);
        perror ("error");
        return;
    }

    shread.block_num = 2;
    shread.buf = &block;
    if (do_io(shd, SHDREADBLOCK, &shread))
    {
        printf ("Error SHDREADBLOCK block 2\n");
        perror ("error");
        return (1); 
    }

    read_start = (long) block[(2*version - 2)]; 
    num_blocks = (long) block[(2*version - 1)];
    write (fd_write_mdata, &read_start, sizeof (long));
    write (fd_write_mdata, &num_blocks, sizeof (long));
    for (ix = 0; ix < num_blocks; ix++)
    {
        shread.block_num = read_start;
        if (do_io(shd, SHDREADBLOCK, &shread))
                return (1);
        for (i = 0; i < 126; i+=3)
        {
             if (0 == block [i])
                 break;
             off = lseek (fd_read,  block[i+1] * BLOCKSIZE, SEEK_SET);
             size = read (fd_read, &buffer, block[i+2] * BLOCKSIZE);
             if (size < 0)
             { 
                 printf ("Error reading block %ld\n", block[i+1]);
                 perror ("error");
             }
             size = write (fd_write_data, &buffer, size);
             if (size < 0)
             {
                 printf ("Error writing block %ld\n", block[i+1]);
                 perror ("error");
             }
             for (j = 0; j < MAXBUF; j++)
                 buffer[j] = 0;
             write (fd_write_mdata, &block[i], sizeof (long));
             write (fd_write_mdata, &block[i+1], sizeof (long));
             write (fd_write_mdata, &block[i+2], sizeof (long));
        } 
        read_start++;
    }

    close (fd_read);
    close (fd_write_data);
    close (fd_write_mdata);
}

int load_checkpoint (int version)
{
    unsigned long int setpos = 0;
    char buffer [MAXBUF];
    long metadata_block [128];
    off_t off;
    int i;   
    int ix;
    int fd_read_data; 
    int fd_read_mdata;
    int fd_write_data;
    int size;    
    int write_start; 
    int num_blocks;
    long byte_count = 0;
    long block_count = 0;
    long chunk_size;
    long key;
    long value;
    char path[256];
    char ver_buf[2];
    char *ver = &ver_buf;
    ver = itoa (version); 
    strcpy (path, "/proj/tbres/images/image");
    strcat (path, ver);
    strcat (path, ".data");

    for (i = 0; i < 128; i++)
        metadata_block[i] = 0;
    fd_write_data = open ("/dev/ad0s1", O_RDWR);
    if (fd_write_data < 0)
    {        
        printf ("Error opening file /dev/ad0s1 O_RDWR\n");
        perror ("error");
        return;
    }

    /* Open the data file to read from */
    
    fd_read_data = open (path, O_RDWR);
    if (fd_read_data < 0)
    {
        printf ("Error opening file %s O_RDWR\n", path);
        perror ("error");
        return;
    }

    /* Open the metadata file to read from */

    strcpy (path, "/proj/tbres/images/image");
    strcat (path, ver);
    strcat (path, ".mdata");

    fd_read_mdata = open (path, O_RDWR);
    if (fd_read_mdata < 0)
    {
        printf ("Error opening file %s O_RDWR\n", path);
        perror ("error");
        return;
    }

    for (i = 0; i < MAXBUF; i++)
        buffer[i] = 0;

    read (fd_read_mdata, &write_start, sizeof (long)); 
    read (fd_read_mdata, &num_blocks, sizeof (long));
    
    while (read (fd_read_mdata, &key, sizeof (long))) 
    {
        read (fd_read_mdata, &value, sizeof (long));
        read (fd_read_mdata, &chunk_size, sizeof (long)); 
        setpos = key * BLOCKSIZE;
        off = lseek (fd_write_data, setpos, SEEK_SET); 
        size = read (fd_read_data, &buffer, chunk_size * BLOCKSIZE);
        if (size < 0)
        {
            printf ("Error reading block %ld\n", key);
            perror ("error"); 
        } 
        size = write (fd_write_data, &buffer, size);
        if (size < 0)
        {
            perror ("error"); 
            printf ("Error writing block %ld\n", key);
        }  
    }
    close (fd_write_data);
    close (fd_read_data);
    close (fd_read_mdata);
}

/* Local Variables: */
/* c-argdecl-indent: 8 */
/* c-indent-level: 8 */
/* End: */
