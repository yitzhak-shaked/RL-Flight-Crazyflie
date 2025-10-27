import * as THREE from "./lib/three.module.js"

class PositionMarkers {
    constructor() {
        this.markers = new THREE.Group()
        this.targetPosition = null // Will be loaded from server
        this.obstacleMarkers = []  // Track obstacle markers separately
        this.createInitialSpawnAreaMarker()
        this.loadTargetPositionFromServer()
    }

    async loadTargetPositionFromServer() {
        try {
            const response = await fetch('/config')
            const config = await response.json()
            this.targetPosition = [
                config.targetPosition.x,
                config.targetPosition.y,
                config.targetPosition.z
            ]
            console.log('Loaded target position from server:', this.targetPosition)
            
            // Load obstacles
            this.obstacles = config.obstacles || []
            console.log('Loaded obstacles from server:', this.obstacles)
            
            this.createTargetPositionMarkers()
            this.createObstacles()
        } catch (error) {
            console.error('Failed to fetch target position from server, using fallback:', error)
            // Fallback to a default position if server config fails
            this.targetPosition = [0.0, 0.0, 0.0]
            this.obstacles = []
            this.createTargetPositionMarkers()
        }
    }

    createInitialSpawnAreaMarker() {
        // Create a wireframe cube to show the initial spawn area (±0.2m around origin)
        const spawnAreaSize = 0.4 // 0.2m * 2 = 0.4m cube
        const geometry = new THREE.BoxGeometry(spawnAreaSize, spawnAreaSize, spawnAreaSize)
        const edges = new THREE.EdgesGeometry(geometry)
        const material = new THREE.LineBasicMaterial({ 
            color: 0x00ff00, // Green for spawn area
            linewidth: 2,
            transparent: true,
            opacity: 0.6
        })
        
        this.spawnAreaMarker = new THREE.LineSegments(edges, material)
        this.spawnAreaMarker.position.set(0, 0, 0)
        this.markers.add(this.spawnAreaMarker)

        // Add a small sphere at the origin to mark the center
        const originGeometry = new THREE.SphereGeometry(0.02, 8, 6)
        const originMaterial = new THREE.MeshBasicMaterial({ 
            color: 0x00ff00,
            transparent: true,
            opacity: 0.8
        })
        this.originMarker = new THREE.Mesh(originGeometry, originMaterial)
        this.originMarker.position.set(0, 0, 0)
        this.markers.add(this.originMarker)

        // Add text label for spawn area
        this.addTextLabel("SPAWN AREA\n(90% random ±0.2m, 10% at origin)", 0, -0.25, 0, 0x00ff00)
    }

    createTargetPositionMarkers() {
        if (!this.targetPosition) {
            console.warn('Target position not loaded yet, skipping marker creation')
            return
        }

        // Target positions from the reward functions
        // BASIC TARGET position comes from server config (src/constants.h)
        const targets = [
            { pos: this.targetPosition, name: "BASIC TARGET", color: 0xff0000, radius: 0.2 },
            { pos: [2.0, 0.0, 1.0], name: "AGGRESSIVE TARGET", color: 0xff6600, radius: 0.15 },
            { pos: [0.5, 0.5, 0.8], name: "CONSERVATIVE TARGET", color: 0xff9999, radius: 0.3 }
        ]

        targets.forEach((target, index) => {
            // Create a sphere for the target
            const targetGeometry = new THREE.SphereGeometry(0.05, 16, 12)
            const targetMaterial = new THREE.MeshBasicMaterial({ 
                color: target.color,
                transparent: true,
                opacity: 0.8
            })
            const targetSphere = new THREE.Mesh(targetGeometry, targetMaterial)
            targetSphere.position.set(target.pos[0], target.pos[1], target.pos[2])
            this.markers.add(targetSphere)

            // Create a wireframe sphere for the success radius
            const radiusGeometry = new THREE.SphereGeometry(target.radius, 16, 12)
            const radiusEdges = new THREE.EdgesGeometry(radiusGeometry)
            const radiusMaterial = new THREE.LineBasicMaterial({ 
                color: target.color,
                transparent: true,
                opacity: 0.3,
                linewidth: 1
            })
            const radiusMarker = new THREE.LineSegments(radiusEdges, radiusMaterial)
            radiusMarker.position.set(target.pos[0], target.pos[1], target.pos[2])
            this.markers.add(radiusMarker)

            // Add text label for target
            this.addTextLabel(
                target.name + `\\n(${target.pos[0]}, ${target.pos[1]}, ${target.pos[2]})`,
                target.pos[0], 
                target.pos[1] + 0.15, 
                target.pos[2],
                target.color
            )
        })
    }

    createObstacles() {
        console.log('createObstacles called with:', this.obstacles)
        if (!this.obstacles || this.obstacles.length === 0) {
            console.log('No obstacles to create')
            return
        }
        
        console.log(`Creating ${this.obstacles.length} obstacle(s)`)

        this.obstacles.forEach((obstacle, index) => {
            console.log(`Processing obstacle ${index}:`, obstacle)
            if (obstacle.type === 'cylinder') {
                // Create cylindrical pipe obstacle as a HOLLOW TUBE
                const height = obstacle.zMax - obstacle.zMin
                
                // Create outer cylinder
                const outerGeometry = new THREE.CylinderGeometry(
                    obstacle.radius,  // radiusTop
                    obstacle.radius,  // radiusBottom
                    height,           // height
                    32,               // radialSegments
                    1,                // heightSegments
                    true              // openEnded = true for hollow tube
                )
                
                // Semi-transparent red material for danger/obstacle
                const tubeMaterial = new THREE.MeshBasicMaterial({
                    color: 0xff0000,
                    transparent: true,
                    opacity: 0.3,
                    side: THREE.DoubleSide  // Render both sides for hollow tube
                })
                
                const tube = new THREE.Mesh(outerGeometry, tubeMaterial)
                
                // Position the tube (center is at y=0 for cylinder, so offset by average)
                const centerZ = (obstacle.zMin + obstacle.zMax) / 2
                tube.position.set(obstacle.x, obstacle.y, centerZ)
                
                // Rotate tube to be vertical (along Z axis)
                // THREE.js cylinder is vertical along Y by default, we need it along Z
                tube.rotation.x = Math.PI / 2
                
                // Mark as obstacle for visibility control
                tube.userData.isObstacle = true
                this.obstacleMarkers.push(tube)
                this.markers.add(tube)
                
                // Add wireframe edges for better visibility of the hollow tube
                const edges = new THREE.EdgesGeometry(outerGeometry)
                const edgeMaterial = new THREE.LineBasicMaterial({
                    color: 0xff0000,
                    transparent: true,
                    opacity: 0.8,
                    linewidth: 2
                })
                const wireframe = new THREE.LineSegments(edges, edgeMaterial)
                wireframe.position.copy(tube.position)
                wireframe.rotation.copy(tube.rotation)
                wireframe.userData.isObstacle = true
                this.obstacleMarkers.push(wireframe)
                this.markers.add(wireframe)
                
                // Add position marker at the base (x, y, z_min)
                const baseMarkerGeometry = new THREE.SphereGeometry(0.05, 16, 12)
                const baseMarkerMaterial = new THREE.MeshBasicMaterial({
                    color: 0xffff00,  // Yellow for base position
                    transparent: true,
                    opacity: 0.9
                })
                const baseMarker = new THREE.Mesh(baseMarkerGeometry, baseMarkerMaterial)
                baseMarker.position.set(obstacle.x, obstacle.y, obstacle.zMin)
                baseMarker.userData.isObstacle = true
                this.obstacleMarkers.push(baseMarker)
                this.markers.add(baseMarker)
                
                // Add position marker at the center (x, y, center_z)
                const centerMarkerGeometry = new THREE.SphereGeometry(0.05, 16, 12)
                const centerMarkerMaterial = new THREE.MeshBasicMaterial({
                    color: 0xff00ff,  // Magenta for center position
                    transparent: true,
                    opacity: 0.9
                })
                const centerMarker = new THREE.Mesh(centerMarkerGeometry, centerMarkerMaterial)
                centerMarker.position.set(obstacle.x, obstacle.y, centerZ)
                centerMarker.userData.isObstacle = true
                this.obstacleMarkers.push(centerMarker)
                this.markers.add(centerMarker)
                
                // Add prominent text label at the top showing PIPE POSITION
                this.addTextLabel(
                    `PIPE OBSTACLE ${index + 1}\\nPosition: (${obstacle.x.toFixed(1)}, ${obstacle.y.toFixed(1)})\\nRadius: ${obstacle.radius}m\\nHeight: Z∈[${obstacle.zMin}, ${obstacle.zMax}]`,
                    obstacle.x,
                    obstacle.y,
                    obstacle.zMax + 0.2,
                    0xff0000
                )
                
                // Add base position label (yellow marker)
                this.addTextLabel(
                    `Base\\n(${obstacle.x.toFixed(1)}, ${obstacle.y.toFixed(1)}, ${obstacle.zMin})`,
                    obstacle.x + 0.15,
                    obstacle.y + 0.15,
                    obstacle.zMin,
                    0xffff00
                )
                
                // Add center position label (magenta marker)
                this.addTextLabel(
                    `Center\\n(${obstacle.x.toFixed(1)}, ${obstacle.y.toFixed(1)}, ${centerZ.toFixed(1)})`,
                    obstacle.x - 0.15,
                    obstacle.y - 0.15,
                    centerZ,
                    0xff00ff
                )
                
                console.log(`Created cylindrical obstacle at (${obstacle.x}, ${obstacle.y}) with radius ${obstacle.radius}m`)
            }
        })
    }

    addTextLabel(text, x, y, z, color) {
        // Create a simple text sprite (fallback since we don't have font loading)
        const canvas = document.createElement('canvas')
        const context = canvas.getContext('2d')
        canvas.width = 256
        canvas.height = 128
        
        context.fillStyle = `#${color.toString(16).padStart(6, '0')}`
        context.font = '16px Arial'
        context.textAlign = 'center'
        context.textBaseline = 'middle'
        
        // Handle multi-line text
        const lines = text.split('\\n')
        lines.forEach((line, index) => {
            context.fillText(line, canvas.width / 2, (canvas.height / 2) + (index - lines.length/2 + 0.5) * 20)
        })
        
        const texture = new THREE.CanvasTexture(canvas)
        const material = new THREE.SpriteMaterial({ 
            map: texture,
            transparent: true,
            opacity: 0.8
        })
        const sprite = new THREE.Sprite(material)
        sprite.position.set(x, y, z)
        sprite.scale.set(0.2, 0.1, 1)
        this.markers.add(sprite)
    }

    get() {
        return this.markers
    }

    setVisibility(visible) {
        this.markers.visible = visible
    }

    // Show only the current target based on training mode
    showTargetForMode(mode) {
        if (!this.targetPosition) {
            console.warn('Target position not loaded yet, cannot show target for mode')
            return
        }

        // Hide all target-related markers first
        this.markers.children.forEach(child => {
            // Keep spawn area, origin, and obstacles always visible
            if (child === this.spawnAreaMarker || child === this.originMarker || child.userData.isObstacle) {
                child.visible = true
                return
            }
            
            // Show/hide targets based on mode
            if (mode === "position-to-position") {
                // Show only the basic target (from constants.h) for position-to-position mode
                const isNearBasicTarget = Math.abs(child.position.x - this.targetPosition[0]) < 0.01 && 
                                        Math.abs(child.position.y - this.targetPosition[1]) < 0.01 && 
                                        Math.abs(child.position.z - this.targetPosition[2]) < 0.01
                child.visible = isNearBasicTarget
            } else {
                // For hover mode, hide all targets
                child.visible = false
            }
        })
        
        // Always show spawn area, origin, and obstacles for both modes
        if (this.spawnAreaMarker) this.spawnAreaMarker.visible = true
        if (this.originMarker) this.originMarker.visible = true
        this.obstacleMarkers.forEach(marker => marker.visible = true)
    }
}

export { PositionMarkers }
