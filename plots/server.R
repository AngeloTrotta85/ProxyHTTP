library(shiny)

# Define server logic required to draw a histogram
shinyServer(
	function(input, output) {
		camps <- list.files("data")
		output$campSelector <- renderUI({
			   selectInput("var", "Choose Option:", as.list(camps)) 
		})

		df = read.table("data/testProvaOK.log-BigBuckBunny_2sec_parsed")
		print(df)
		dd = data.frame(x = df$V6, y = df$V3)
		
		output$chart <- renderPlot({
			plot(dd)
		})
})
