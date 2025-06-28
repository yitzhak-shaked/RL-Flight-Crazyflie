import * as THREE from "./lib/three.module.js"

class PositionMarkers {
    constructor() {
        this.markers = new THREE.Group()
        this.targetPosition = null // Will be loaded from server
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
            this.createTargetPositionMarkers()
        } catch (error) {
            console.error('Failed to fetch target position from server, using fallback:', error)
            // Fallback to a default position if server config fails
            this.targetPosition = [0.0, 0.0, 0.0]
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
            // Keep spawn area and origin always visible
            if (child === this.spawnAreaMarker || child === this.originMarker) {
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
        
        // Always show spawn area for both modes
        if (this.spawnAreaMarker) this.spawnAreaMarker.visible = true
        if (this.originMarker) this.originMarker.visible = true
    }
}

export { PositionMarkers }
