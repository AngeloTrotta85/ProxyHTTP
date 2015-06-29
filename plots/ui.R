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
      
      selectInput("var2", 
        label = "Choose metric to plot",
        choices = list("Bitrate", "Buffer"),
        selected = "Bitrate")
      
    ),
    
    mainPanel(plotOutput("chart"))
  )
))
