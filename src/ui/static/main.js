import * as THREE from "./lib/three.module.js"
import {Drone} from "./drone.js"
import {CoordinateSystem} from "./coordinate_system.js"
import {Simulator} from "./simulator.js"
import {default_model} from "./default_model.js"
import {PositionMarkers} from "./position_markers.js"


window.onload = function(){
  let parameters = {
    "capture": false,
    "width": 1920,
    "height": 1080,
    "cameraDistanceToOrigin": 1,
  }


  var canvasContainer = document.getElementById("canvasContainer")
  let simulator = new Simulator(canvasContainer, parameters)
  window.simulator = simulator

  // Create and add position markers
  window.positionMarkers = new PositionMarkers()
  simulator.add(window.positionMarkers.get())
  window.positionMarkers.setVisibility(false) // Hidden initially

  // Update legend with target position once loaded
  function updateLegendTargetPosition() {
    if (window.positionMarkers.targetPosition) {
      const targetElement = document.getElementById('target-position')
      if (targetElement) {
        const pos = window.positionMarkers.targetPosition
        targetElement.textContent = `(${pos[0]}, ${pos[1]}, ${pos[2]})`
      }
    } else {
      // Retry after a short delay if not loaded yet
      setTimeout(updateLegendTargetPosition, 100)
    }
  }
  updateLegendTargetPosition()

  window.drones = {}
  window.origin_coordinate_systems = {}

  window.removeDrone = id => {
    if(id in window.drones){
      // console.log("Removing drone")
      simulator.remove(window.drones[id].get())
      delete window.drones[id]
    }
    if(id in window.origin_coordinate_systems){
      // console.log("Removing origin frame")
      simulator.remove(window.origin_coordinate_systems[id].get())
      delete window.origin_coordinate_systems[id]
    }
  }
  window.addDrone = (
    id,
    origin,
    model,
    {
      displayGlobalCoordinateSystem = true,
      displayIMUCoordinateSystem = true,
      displayActions = true
    }
  ) => {
    window.removeDrone(id)
    window.drones[id] = new Drone(model, origin, null, displayIMUCoordinateSystem, displayActions)
    // drone.get().position.set(0, 0.2, 0.2)
    simulator.add(window.drones[id].get())
    if (displayGlobalCoordinateSystem){
      let cs = new CoordinateSystem(origin, 1, 0.01)
      simulator.add(cs.get())
      window.origin_coordinate_systems[id] = cs
    }
    window
  }

  window.setInfo = (info) => {
    const infoContainer = document.getElementById("infoContainer")
    infoContainer.style.display = "block"
    infoContainer.innerHTML = info
  }

  function onWindowResize(){
    if (!capture){
      const width = canvasContainer.offsetWidth
      const height = canvasContainer.offsetHeight
      simulator.camera.aspect =  width / height
      simulator.camera.updateProjectionMatrix()
      simulator.renderer.setSize(width, height)
    }
  }
  onWindowResize()
  window.addEventListener('resize', onWindowResize, false);



  var buttonHover = document.getElementById("button-hover")
  var buttonPosition = document.getElementById("button-position")
  var buttonEvaluate = document.getElementById("button-evaluate")
  var buttonEvalRestart = document.getElementById("button-eval-restart")
  var buttonEvalStop = document.getElementById("button-eval-stop")
  var buttonEvalExit = document.getElementById("button-eval-exit")
  var actorSelect = document.getElementById("actor-select")
  var evaluateActorContainer = document.getElementById("evaluateActorContainer")
  var controlContainer = document.getElementById("controlContainer")
  var seed_input = document.getElementById("seed-input")
  
  // Actor evaluation state
  var isEvaluating = false
  var availableActors = []
  var currentEvaluationMode = null
  
  // Load available actors from server
  async function loadAvailableActors() {
    try {
      const response = await fetch('/actors')
      if (!response.ok) {
        throw new Error(`HTTP error! status: ${response.status}`)
      }
      const actors = await response.json()
      availableActors = actors
      updateActorSelect()
    } catch (error) {
      console.error('Failed to load actors:', error)
      actorSelect.innerHTML = '<option value="">Error loading actors</option>'
    }
  }
  
  // Update actor select dropdown
  function updateActorSelect() {
    if (availableActors.length === 0) {
      actorSelect.innerHTML = '<option value="">No actors available</option>'
      return
    }
    
    let html = '<option value="">Select an actor...</option>'
    availableActors.forEach(actor => {
      html += `<option value="${actor.path}">${actor.name}</option>`
    })
    actorSelect.innerHTML = html
  }
  
  // Show actor evaluation UI
  function showActorEvaluationUI() {
    controlContainer.style.display = 'none'
    evaluateActorContainer.style.display = 'block'
    
    loadAvailableActors()
    
    // Show position markers for evaluation
    window.positionMarkers.setVisibility(true)
    window.positionMarkers.showTargetForMode("position-to-position")
    document.getElementById("legendContainer").style.display = "block"
  }
  
  // Hide actor evaluation UI and return to main menu
  function hideActorEvaluationUI() {
    evaluateActorContainer.style.display = 'none'
    controlContainer.style.display = 'block'
    isEvaluating = false
    currentEvaluationMode = null
    
    // Hide position markers
    window.positionMarkers.setVisibility(false)
    document.getElementById("legendContainer").style.display = "none"
    
    // Reset button states
    buttonHover.disabled = false
    buttonPosition.disabled = false
    buttonEvaluate.disabled = false
    seed_input.style.display = "block"
  }
  var animateButton = function(e) {
    e.preventDefault;
    //reset animation
    e.target.classList.remove('animate');

    e.target.classList.add('animate');
    setTimeout(function(){
      e.target.classList.remove('animate');
    },700);
  };

  buttonHover.addEventListener('click', animateButton, false);
  buttonPosition.addEventListener('click', animateButton, false);
  buttonEvaluate.addEventListener('click', animateButton, false);
  buttonEvalRestart.addEventListener('click', animateButton, false);
  buttonEvalStop.addEventListener('click', animateButton, false);
  buttonEvalExit.addEventListener('click', animateButton, false);


  document.addEventListener("keypress", function onPress(event) {
    if (event.key === "u" && event.ctrlKey) {
      document.getElementById("controlContainer").style.display = document.getElementById("controlContainer").style.display == "none" ? "block" : "none"
    }
  });

  window.getFrame = function(){
    return renderer.domElement.toDataURL("image/png");
  }

  var ws = new WebSocket('ws://' + window.location.host + "/ws");

  ws.onopen = function(event) {
    console.log('Connection opened:', event);
  };

  ws.onmessage = function(event) {
    let {channel, data} = JSON.parse(event.data)
    if (channel === "addDrone") {
      window.addDrone(data.id, data.origin, data.model || default_model, data.display_options);
    }
    else{
      if (channel === "setDroneState") {
        if(data.id in window.drones){
          window.drones[data.id].setState(data.data)
        }
        else{
          throw new Error("Drone not found")
        }
      }
      else{
        if (channel === "removeDrone") {
          window.removeDrone(data.id)
        }
        else{
          if (channel === "status") {
            if (isEvaluating) {
              // Update evaluation info
              const evalInfo = document.getElementById("evaluateInfoContainer")
              evalInfo.innerHTML = `<p style="font-family: Arial, sans-serif;">Evaluating actor...</p>`
            } else {
              // Update training progress
              const progressText = "Training progress: " + Math.round(data.progress * 100) + "%"
              buttonHover.innerHTML = progressText
              buttonPosition.innerHTML = progressText
              if(data.finished && !window.finished){
                window.finished = true;
                document.getElementById("canvasContainer").style.display = "none"
                document.getElementById("controlContainer").style.display = "none"
                var resultContainer = document.getElementById("resultContainer")
                resultContainer.innerHTML = "Finished"
                resultContainer.style.display = "block"
              }
            }
          }
          else{
            if (channel === "evaluationStopped") {
              // Reset evaluation UI state
              const evalInfo = document.getElementById("evaluateInfoContainer")
              evalInfo.innerHTML = ""
              isEvaluating = false
            }
          }
        }
      }
    }
  };

  ws.onerror = function(error) {
    console.error('WebSocket Error:', error);
  };

  ws.onclose = function(event) {
    if (event.wasClean) {
      console.log('Connection closed cleanly, code=', event.code, 'reason=', event.reason);
    } else {
      console.error('Connection died');
    }
  };

  buttonHover.addEventListener("click", event => {
    seed_input.style.display = "none"
    buttonHover.disabled = true;
    buttonPosition.disabled = true;
    
    // Show position markers for hover mode
    window.positionMarkers.setVisibility(true)
    window.positionMarkers.showTargetForMode("hover")
    
    // Show legend
    document.getElementById("legendContainer").style.display = "block"
    
    ws.send(JSON.stringify({
      "channel": "startTraining",
      "data": {
        "seed": parseInt(seed_input.value || 0),
        "mode": "hover"
      }
    }))
  }, false)

  buttonPosition.addEventListener("click", event => {
    seed_input.style.display = "none"
    buttonHover.disabled = true;
    buttonPosition.disabled = true;
    
    // Show position markers for position-to-position mode
    window.positionMarkers.setVisibility(true)
    window.positionMarkers.showTargetForMode("position-to-position")
    
    // Show legend
    document.getElementById("legendContainer").style.display = "block"
    
    ws.send(JSON.stringify({
      "channel": "startTraining",
      "data": {
        "seed": parseInt(seed_input.value || 0),
        "mode": "position-to-position"
      }
    }))
  }, false)
  
  // Actor evaluation button handlers
  buttonEvaluate.addEventListener("click", event => {
    showActorEvaluationUI()
  }, false)
  
  buttonEvalRestart.addEventListener("click", event => {
    const selectedActor = actorSelect.value
    if (!selectedActor) {
      alert("Please select an actor first")
      return
    }
    
    isEvaluating = true
    const evalInfo = document.getElementById("evaluateInfoContainer")
    evalInfo.innerHTML = `<p style="font-family: Arial, sans-serif;">Starting evaluation with actor: ${selectedActor}</p>`
    
    // Remove existing drones
    Object.keys(window.drones).forEach(id => {
      window.removeDrone(id)
    })
    
    ws.send(JSON.stringify({
      "channel": "evaluateActor",
      "data": {
        "actorPath": selectedActor,
        "action": "restart"
      }
    }))
  }, false)
  
  buttonEvalStop.addEventListener("click", event => {
    if (isEvaluating) {
      ws.send(JSON.stringify({
        "channel": "evaluateActor",
        "data": {
          "action": "stop"
        }
      }))
      
      // Remove existing drones
      Object.keys(window.drones).forEach(id => {
        window.removeDrone(id)
      })
      
      const evalInfo = document.getElementById("evaluateInfoContainer")
      evalInfo.innerHTML = ""
      isEvaluating = false
    }
  }, false)
  
  buttonEvalExit.addEventListener("click", event => {
    if (isEvaluating) {
      ws.send(JSON.stringify({
        "channel": "evaluateActor",
        "data": {
          "action": "stop"
        }
      }))
      
      // Remove existing drones
      Object.keys(window.drones).forEach(id => {
        window.removeDrone(id)
      })
    }
    
    hideActorEvaluationUI()
  }, false)
}