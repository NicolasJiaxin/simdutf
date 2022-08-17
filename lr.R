
sizes=c(16, 32, 64, 100, 250, 500, 1024, 2048, 4096, 10240)
times=c(400, 500, 600, 700, 1100, 1800, 3300, 6300, 12400, 30700)
timesiconv=c(500, 600, 900, 1200, 2400, 4400, 8600, 16800, 33300, 82600)
model <- lm(times~sizes)
modeliconv <- lm(timesiconv~sizes)
summary(model)
summary(modeliconv)
plot(sizes, times, col='forestgreen', main="Random UTF-8 to UTF-16LE", xlab="UTF-8 size(KB)", ylab="Time(μs)")
points(sizes, timesiconv, col='blue')
abline(lm(times~sizes),col = 'forestgreen',lwd = 3)
abline(lm(timesiconv~sizes),col = 'blue',lwd = 3)
legend("bottomright", legend=c("sutf", "iconv"),
       col=c("forestgreen", "blue"), lty=1:2, cex=0.8)
