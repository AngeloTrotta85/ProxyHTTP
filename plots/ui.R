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
        choices = list("TIME" = "V1", "SEGMENT" = "V6"),
		selected = "V5"),

      selectInput("ycol", 
        label = "Choose metric to plot",
        choices = list("Bitrate" = "V3", "Buffer" = "V17", "SIZE" = "V8", "THROUGHPUT" = "V14", "PAUSE" = "V16"),
        selected = "V3")
      
    ),
    
    mainPanel(
    	verbatimTextOutput("summary"),
		tableOutput("view"),
		plotOutput("chart")
	)
  )
))
