pdf("test2.pdf")
d <- ggplot(data=mydataf_caba,aes(x=V6,y=V3))
d <- d + geom_line(colour="blue")
d <- d + geom_line(data=mydataf_cabafixed,aes(x=V6,y=V3),colour="red")
d <- d + geom_line(data=mydataf_sbs,aes(x=V6,y=V3),colour="green")
d <- d + geom_line(data=mydataf_wifi,aes(x=V6,y=V3),colour="yellow")
d <- d + theme_bw()
d <- d + scale_x_continuous(expand=c(0,0))
d

pdf("CABA_size.pdf")
d <- ggplot(data=mydataf_caba,aes(x=V1,y=V3))
d <- d + geom_line(colour="blue")
d <- d + geom_line(data=mydataf_caba_2,aes(x=V1,y=V3),colour="red")
d <- d + geom_line(data=mydataf_caba_4,aes(x=V1,y=V3),colour="green")
d <- d + geom_line(data=mydataf_caba_6,aes(x=V1,y=V3),colour="yellow")
d <- d + geom_line(data=mydataf_caba_10,aes(x=V1,y=V3),colour="black")
d <- d + geom_line(data=mydataf_caba_15,aes(x=V1,y=V3),colour="purple")
d <- d + theme_bw()
d <- d + scale_x_continuous(expand=c(0,0))
d

pdf("fixed_offset.pdf")
d <- ggplot(data=mydataf_cabafixed_2,aes(x=V6,y=V3))
d <- d + geom_line(colour="blue")
d <- d + geom_line(data=mydataf_cabafixed_4,aes(x=V6,y=V3),colour="green")
d <- d + geom_line(data=mydataf_cabafixed_6,aes(x=V6,y=V3),colour="yellow")
d <- d + geom_line(data=mydataf_cabafixed_8,aes(x=V6,y=V3),colour="red")
d <- d + geom_line(data=mydataf_cabafixed_10,aes(x=V6,y=V3),colour="black")
d <- d + theme_bw()
d <- d + scale_x_continuous(expand=c(0,0))
d <- d + scale_y_continuous(expand=c(0,0),limits=c(0,5000000))
d

mydf_avg <- data.frame()
print("CABA")
df_tmp <- colMeans(mydataf_caba['V3']) / 1024
df_tmp['algo'] = 'caba'
mydf_avg = df_tmp

print("CABA FIXED")
df_tmp <- colMeans(mydataf_cabafixed['V3']) / 1024
df_tmp['algo'] = "fixed"
mydf_avg = rbind(mydf_avg,df_tmp)

print("STEP BY STEP")
df_tmp <- colMeans(mydataf_sbs['V3']) / 1024
df_tmp['algo'] = "SbS"
mydf_avg = rbind(mydf_avg,df_tmp)

print("WIFI")
df_tmp <- colMeans(mydataf_sbs['V3']) / 1024
df_tmp['algo'] = "WiFi"
mydf_avg = rbind(mydf_avg,df_tmp)

mydf_avg


pdf("avgQual.pdf")
d <- ggplot(data=as.data.frame(mydf_avg),aes(x=algo, y=V3,fill=factor(algo)))
d <- d + geom_bar(binwidth = 1, drop = FALSE, right = TRUE,stat="identity")
d <- d + theme_bw()
d

mydataf_cabafixed_2['offset'] = 2
mydataf_cabafixed_4['offset'] = 4
mydataf_cabafixed['offset'] = 5
mydataf_cabafixed_6['offset'] = 6
mydataf_cabafixed_8['offset'] = 8
mydataf_cabafixed_10['offset'] = 10


mydf_avg_off <- rbind(colMeans(mydataf_cabafixed_2),colMeans(mydataf_cabafixed_4))
#mydf_avg_off <- rbind(mydf_avg_off,colMeans(mydataf_cabafixed))
mydf_avg_off <- rbind(mydf_avg_off,colMeans(mydataf_cabafixed_6))
mydf_avg_off <- rbind(mydf_avg_off,colMeans(mydataf_cabafixed_8))
mydf_avg_off <- rbind(mydf_avg_off,colMeans(mydataf_cabafixed_10))
pdf("avgQualOffset.pdf")
d <- ggplot(data=as.data.frame(mydf_avg_off),aes(x=offset, y=V3,fill=factor(offset)))
d <- d + geom_bar(binwidth = 1, drop = FALSE, right = TRUE,stat="identity")
d <- d + theme_bw()
d
