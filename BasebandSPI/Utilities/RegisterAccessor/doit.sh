sudo xcodebuild build $BULLSHIT -target 'All' -configuration 'Release' build install && \
sudo ditto /tmp/RegisterAccessor.dst/ /Developer/SDKs/Purple/ && \
rsync -av  /tmp/RegisterAccessor.dst/usr/* $RSYNCP2/usr  && \
rsync -av build $RSYNCP2/`pwd` && \
mfc && \
sudo cp /private/tftpboot/arjuna.kernelcache.im3 ../  && \
ditto .. /Volumes/DropZone/People/Arjuna/SPI_Utilities &&\
ditto /tmp/RegisterAccessor.dst/usr /Volumes/DropZone/People/Arjuna/SPI_Utilities/usr 
