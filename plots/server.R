library(shiny)

# Define server logic required to draw a histogram
shinyServer(
	function(input, output) {
		camps <- list.files("data/", pattern = "*parsed*")
		output$campSelector <- renderUI({
			   selectInput("vaar", "Choose Option:", as.list(camps)) 
		})

		dataf <- as.data.frame(read.table("data/test-BigBuckBunny_1sec_parsed"))
		dataInput <- reactive({
#			read.table("data/testProvaOK.log-BigBuckBunny_2sec_parsed")
			read.table(paste("data/",input$vaar,sep = ""))
#			print(input$vaar)
#			print(input$var2)
#			dataf <- as.data.frame(dataInput())
		})
		selectedData <- reactive({
#			print(input$xcol)
#			print(input$ycol)
#			print(XX)
#			print(dataf[,c(input$xcol)])
			dataf <- dataInput()
			print(dataf)
			dataf[, c(input$xcol, input$ycol)]
		})

#		print(dataInput())
#		df = read.table(paste("data",var))
#		print(df)
#		dd = data.frame(x = df$V6, y = df$V3)
		
		output$chart <- renderPlot({
			plot(selectedData(), type="line")
		})
})
