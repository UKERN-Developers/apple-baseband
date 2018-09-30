
installsrc:
	ditto . ${SRCROOT}

installhdrs:
	@echo Not applicable

install: ${DSTDIR}
	ditto --norsrc ${SRCROOT}/firmware ${DSTROOT}/usr/local/standalone/firmware
	chown -R root:admin ${DSTROOT}
	find ${DSTROOT} -name ._\* -exec rm -f {} \;

clean:
	find . \( -name '.DS_Store' \) -exec rm -rf {} \;


