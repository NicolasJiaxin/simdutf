library(ggplot2)

sizes=c(16, 32, 64, 100, 250, 500, 1024, 2048, 4096, 10240)
times=c(0.4, 0.5, 0.6, 0.7, 1.1, 1.8, 3.3, 6.3, 12.4, 30.7)
sutf <- data.frame(sizes, times)
scatter.smooth(sizes, times, main='Hours studied vs. Exam Score')
head(sutf)
model <- lm(times~sizes)
summary(model)
graph <- ggplot(sutf, aes(x=sizes, y=times))
graph <- graph + geom_smooth(method="lm", col="black")

graph