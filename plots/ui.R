library(shiny)

shinyUI(fluidPage(
  titlePanel("MPEG_DASH Plots"),
  
  sidebarLayout(
    sidebarPanel(
      helpText("Plot video traces."),
      
      uiOutput("campSelector"),

#      selectInput("var", 
#        label = "Choose a file to analyze",
#        choices = list("Percent White", "Percent Black",
#        choices = uiOutput("campSelector"),
#          "Percent Hispanic", "Percent Asian"),
#        selected = "Percent White"),
      
      selectInput("xcol", 
        label = "Choose X",
        choices = list("TIME" = "TS", "SEGMENT" = "FN"),
		selected = "TS"),

         #col.names = c("TS","NAME","BPS","RCV","SEC","FN","INT","BYTE","BIT","START","END","SEND","UNKN","TBYTE","TBIT","PAUSE","BUFF"))

      selectInput("ycol", 
        label = "Choose metric to plot",
        choices = list("Bitrate" = "BPS", "Buffer" = "BUFF", "SIZE" = "BYTE", "THROUGHPUT" = "TBYTE", "PAUSE" = "PAUSE"),
        selected = "BITRATE")
      
    ),
    
    mainPanel(
    	verbatimTextOutput("summary"),
		#tableOutput("view"),
		plotOutput("chart")
	)
  )
))
