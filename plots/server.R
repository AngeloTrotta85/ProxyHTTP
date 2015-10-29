library(shiny)

# Define server logic required to draw a histogram
shinyServer(
	function(input, output, session) {
		autoInvalidate <- reactiveTimer(1000, session)
		camps <- list.files("data/", pattern = "*")
		output$campSelector <- renderUI({
			#TODO Would be nice to give the possibility to plot more files on the same chart
			# This would involve a for loop through the different files
			selectInput("vaar", "Choose Option:", as.list(camps)) 
		})

		dataf <- as.data.frame(read.table(paste("data/",camps[1],sep="")))
		dataInput <- reactive({
			autoInvalidate()
#			read.table("data/testProvaOK.log-BigBuckBunny_2sec_parsed")
			#TODO Add names to the columns
			read.table(paste("data/",input$vaar,sep = ""), 
			col.names = c("TS","NAME","BPS","RCV","SEC","FN","INT","BYTE","BIT","START","END","SEND","UNKN","TBYTE","TBIT","PAUSE","BUFF"))
#			print(input$vaar)
#			print(input$var2)
#			dataf <- as.data.frame(dataInput())
		})

		selectedData <- reactive({
			autoInvalidate()
			print("AAAA")
#			print(input$xcol)
#			print(input$ycol)
#			print(XX)
#			print(dataf[,c(input$xcol)])
			dataf <- dataInput()
			if (input$xcol == "TS")
				dataf = dataf[order(dataf$TS),] 
			#print(dataf)
			#print(input)
			dataf[, c(input$xcol, input$ycol)]
		})

#		print(dataInput())
#		df = read.table(paste("data",var))
#		print(df)
#		dd = data.frame(x = df$V6, y = df$V3)

		output$summary <- renderPrint({
    		dataset <- selectedData()
    		summary(dataset)
  		})

  		output$view <- renderTable({
    		#head(selectedData(), n = input$obs)
    		head(selectedData())
  		})
		
		output$chart <- renderPlot({
			autoInvalidate()
			print("BBBB")
#			print(selectedData())
			plot(selectedData(), type="l", col="red", lwd = 10)
		})
})
