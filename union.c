#include <u.h>
#include <libc.h>

int fatal = 1;

void
usage(void)
{
	fprint(2, "%s: [-abcr] path [...] mtpt\n", argv0);
	exits("usage");
}

void
error(char *fmt, ...)
{
	va_list arg;

	va_start(arg, fmt);
	vfprint(2, fmt, arg);
	va_end(arg);
	if(fatal)
		exits("fatal");
}

int
same(char *p, char *q)
{
	int r;
	Dir *dp, *dq;

	r = 0;
	if((dp = dirstat(p)) == nil)
		error("dirstat: %r\n");
	if((dq = dirstat(q)) == nil)
		error("dirstat: %r\n");
	if(dp->type==dq->type
	&& dp->dev==dq->dev
	&& dp->qid.path==dq->qid.path
	&& dp->qid.vers==dq->qid.vers
	&& dp->qid.type==dq->qid.type)
		r = 1;
	free(dp);
	free(dq);
	return r;
}

void
replace(char *p, char *q)
{
	if((bind(p, q, MREPL)) < 0)
		error("bind %s %s: %r\n", p, q);
}

void
after(char *p, char *q)
{
	int fd;
	long n;
	char np[1024], nq[1024];
	Dir *dir, *d;

	if(same(p, q) == 0)
	if((bind(p, q, MAFTER)) < 0)
		error("bind %s %s: %r\n", p, q);
	if((fd = open(p, OREAD)) < 0)
		sysfatal("open: %r");
	if((n = dirreadall(fd, &dir)) < 0)
		error("dirreadall: %r\n");
	close(fd);
	for(d = dir; n > 0; n--, d++){
		if((d->mode&DMDIR) == 0)
			continue;
		snprint(np, sizeof np, "%s/%s", p, d->name);
		snprint(nq, sizeof nq, "%s/%s", q, d->name);
		after(np, nq);
	}
}

void
before(char *p, char *q)
{
	char *t;
	
	t = "/mnt/temp";
	replace(p, t);
	after(q, t);
	replace(t, q);
	unmount(nil, t);
}

void
main(int argc, char *argv[])
{
	int i;
	char *mtpt;
	int op;

	enum {After, Before, Replace};
	op = After;
	ARGBEGIN{
	default: usage();
	case 'a': op = After; break;
	case 'b': op = Before; break;
	case 'r': op = Replace; break;
	case 'q': fatal = 0; break;
	}ARGEND;
	if(argc < 2)
		usage();

	mtpt = argv[argc-1];
	for(i = 0; i < argc-1; i++)
		switch(op){
		default: usage();
		case After:
			after(argv[i], mtpt); break;
		case Before:
			before(argv[i], mtpt); break;
		case Replace:
			replace(argv[i], mtpt); break;
		}
}
