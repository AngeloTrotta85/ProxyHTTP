library(ggplot2)

mydata_caba <- as.data.frame(read.table('caba/test1_1-BigBuckBunny_1sec_'))
mydata_cabafixed <- as.data.frame(read.table('caba_fixed/test1_1-BigBuckBunny_1sec_'))
mydata_sbs <- as.data.frame(read.table('stepbystep/test1_1-BigBuckBunny_1sec_'))
mydata_wifi <- as.data.frame(read.table('wifi/test1_1-BigBuckBunny_1sec_'))
for (i in 2:14) {
	mydatatmp_caba <- as.data.frame(read.table(paste('caba/test1_',i,'-BigBuckBunny_1sec_',sep="")))
	mydatatmp_cabafixed <- as.data.frame(read.table(paste('caba_fixed/test1_',i,'-BigBuckBunny_1sec_',sep="")))
	mydatatmp_sbs <- as.data.frame(read.table(paste('stepbystep/test1_',i,'-BigBuckBunny_1sec_',sep="")))
	mydatatmp_wifi <- as.data.frame(read.table(paste('wifi/test1_',i,'-BigBuckBunny_1sec_',sep="")))
	mydata_caba <- rbind(mydata_caba,mydatatmp_caba)
	mydata_cabafixed <- rbind(mydata_cabafixed,mydatatmp_cabafixed)
	mydata_sbs <- rbind(mydata_sbs,mydatatmp_sbs)
	mydata_wifi <- rbind(mydata_wifi,mydatatmp_wifi)
}

mydataf_caba <- aggregate(.~V6, data=mydata_caba, mean)
mydataf_cabafixed <- aggregate(.~V6, data=mydata_cabafixed, mean)
mydataf_sbs <- aggregate(.~V6, data=mydata_sbs, mean)
mydataf_wifi <- aggregate(.~V6, data=mydata_wifi, mean)


d <- ggplot(data=mydataf_caba,aes(x=V6,y=V3))
d <- d + geom_line(colour="blue")
d <- d + geom_line(data=mydataf_cabafixed,aes(x=V6,y=V3),colour="red")
d <- d + geom_line(data=mydataf_sbs,aes(x=V6,y=V3),colour="green")
d <- d + geom_line(data=mydataf_wifi,aes(x=V6,y=V3),colour="yellow")
d

