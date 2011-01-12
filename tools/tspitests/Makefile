all:
	gcc -o idkey idkey.c -ltspi #-lcrypto
	gcc -o keygen keygen.c -ltspi
	gcc -o doquote doquote.c -ltspi
	gcc -o tpm-signoff tpm-signoff.c -ltspi
	gcc -o getpub getpub.c -ltspi
	gcc -o loadkey loadkey.c -ltspi
	gcc -o fail fail.c -ltspi
	gcc -o pcrcomposite pcrcomposite.c -ltspi
	gcc -o ltpmloadkey ltpmloadkey.c libtpm/bind.c libtpm/chgauth.c \
	    libtpm/hmac.c libtpm/keys.c libtpm/migrate.c libtpm/miscfunc.c \
	    libtpm/oiaposap.c libtpm/owner.c libtpm/pcrs.c libtpm/seal.c \
	    libtpm/signature.c libtpm/tpmutil.c -Ilibtpm -lcrypto
clean:
	rm -f idkey keygen doquote tpm-signoff getpub loadkey fail pcrcomposite ltpmloadkey
