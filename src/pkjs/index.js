/**
 * Arbitrary dampening factor to account for different atmospheric regions
 */
const regionScoreDampening = {
    north: 1.0,
    south: 0.75,
}

/**
 * Geographic coordinates for observing Mount Fuji from different regions
 * @typedef {Object} RegionCoords
 * @property {Array<{lat: number, long: number, distanceKm: number}>} north Coordinates for northern observation points
 * @property {Array<{lat: number, long: number, distanceKm: number}>} south Coordinates for southern observation points
 * Each array contains 3 points:
 * 1. Observer location (starting point)
 * 2. Mid-point
 * 3. Approach point towards Mount Fuji
 */
const regionCoords = {
    north: [
        { lat: 35.5, long: 138.75, distanceKm: 0 }, // Observer location
        { lat: 35.45, long: 138.75, distanceKm: 5.55 }, // Mid-point
        { lat: 35.4, long: 138.75, distanceKm: 11.09 }, // Approach
    ],
    south: [
        { lat: 35.2, long: 138.6875, distanceKm: 0 }, // Observer location
        { lat: 35.25, long: 138.6875, distanceKm: 5.55 }, // Mid-point
        { lat: 35.3, long: 138.75, distanceKm: 12.47 } // Approach
    ]
}

/**
 * Object representing visibility scores and weights for different regions and times of day.
 */
const regionScores = {
    north: {
        morning: {
            score: 0, weight: 0
        },
        afternoon: {
            score: 0, weight: 0
        }
    },
    south: {
        morning: {
            score: 0, weight: 0
        },
        afternoon: {
            score: 0, weight: 0
        }
    }
}

/**
 * Generates a URL for the Open-Meteo API with weather forecast parameters
 * @param {Object} params - The parameters object
 * @param {number} params.lat - The latitude coordinate
 * @param {number} params.long - The longitude coordinate
 * @param {('morning'|'afternoon')} params.time - The time of day to get forecast for
 * @returns {string} The complete URL for the Open-Meteo API request
 */
function getUrl({ lat, long, time }) {
    const now = new Date()
    now.setHours(now.getHours() + 9)
    const today = now.toISOString().split('T')[0]
    const hourRange = time === 'morning'
        ? { start: `${today}T06:00`, end: `${today}T11:00` }
        : { start: `${today}T12:00`, end: `${today}T17:00` }
    return `https://api.open-meteo.com/v1/forecast?` +
        `latitude=${lat}` +
        `&longitude=${long}` +
        `&hourly=cloud_cover_low,precipitation,weather_code,relative_humidity_2m` +
        `&timezone=Asia/Tokyo` +
        `&start_hour=${hourRange.start}` +
        `&end_hour=${hourRange.end}`
}

/**
 * Calculates a visibility score (0-10) for Mt. Fuji based on weather conditions.
 * @param {Object} params - Weather condition parameters
 * @param {number} params.cloudCoverLow - Cloud cover percentage at low altitude (0-100)
 * @param {number} params.relativeHumidity - Relative humidity percentage (0-100)
 * @param {number} params.weatherCode - WMO weather code
 * @param {number} params.precipitation - Precipitation amount in mm
 * @param {'north'|'south'} params.region - Region from which Fuji is being viewed
 * @returns {number} Visibility score from 1 (not visible) to 10 (perfect visibility)
 */
function calculateVisibilityScore({
    cloudCoverLow,
    relativeHumidity,
    weatherCode,
    precipitation,
    region
}) {
    // Immediate disqualifiers
    if (weatherCode >= 45 && weatherCode <= 48) return 0 // Fog
    if ([65, 67, 75, 77, 95, 96, 99].includes(weatherCode)) return 0 // Heavy precipitation
    if (precipitation > 5.0) return 0 // Heavy rain threshold

    // Base score from cloud cover
    let score = 10 * (1 - cloudCoverLow / 100)

    // Humidity penalty (atmospheric haze)
    const humidityPenalty =
        relativeHumidity > 80 ? 0.3 :
            relativeHumidity > 60 ? 0.7 : 1.0
    score *= humidityPenalty

    // Weather code penalties
    if (weatherCode === 3) score *= 0.4 // Overcast
    else if (weatherCode >= 51 && weatherCode <= 67) score *= 0.6 // Any precipitation

    // Dampening factor for different regions
    score = score * regionScoreDampening[region || 'north']

    return Math.round(Math.max(1, Math.min(10, score)))
}

/**
 * Handles the response from a weather data request and calculates visibility scores
 * @param {Object} params - The parameters object
 * @param {'north'|'south'} params.region - The geographical region to calculate scores for
 * @param {'morning'|'afternoon'} params.time - The time period for the calculation
 * @this {XMLHttpRequest} - The XHR context containing the response data
 * @description This function:
 * 1. Parses weather data response
 * 2. Calculates hourly visibility scores
 * 3. Applies distance-based weighting
 * 4. Updates regional scores
 * 5. Posts final weighted score to Pebble device
 */
function requestOnLoad({ region, time }) {
    if (this.status >= 200 && this.status < 400) {
        const response = JSON.parse(this.response)
        console.log('[PebbleKit JS]: Received valid response')
        const { latitude, longitude } = response
        const currentRegion = regionCoords[region].find(regionCoord => regionCoord.lat === latitude && regionCoord.long === longitude)

        // Calculate the average score for a time span per point of a region
        const { hourly: hourlyWeather } = response
        let hourlyAverageScore = -1
        for (let i = 0; i < hourlyWeather.time.length; i++) {
            const relativeHumidity = hourlyWeather.relative_humidity_2m[i]
            const precipitation = hourlyWeather.precipitation[i]
            const cloudCoverLow = hourlyWeather.cloud_cover_low[i]
            const weatherCode = hourlyWeather.weather_code[i]

            let visibilityScore = calculateVisibilityScore({
                cloudCoverLow,
                relativeHumidity,
                weatherCode,
                precipitation,
                region
            })

            if (hourlyAverageScore === -1) {
                hourlyAverageScore = visibilityScore
            } else {
                hourlyAverageScore = (hourlyAverageScore + visibilityScore) / 2
            }
        }

        // Weigh the score based on the distance to the observer point
        const weight = Math.exp(-0.1 * currentRegion.distanceKm)
        const weightedScore = hourlyAverageScore * weight
        const { score: currentScore, weight: currentWeight } = regionScores[region][time]
        regionScores[region][time] = {
            score: currentScore + weightedScore,
            weight: currentWeight + weight
        }
    } else {
        console.log('[PebbleKit JS]: Received bad response')
    }
}

function calculateWeightedScore({ score, weight }) {
    return weightedScore = Math.round(score / weight)
}

function updateAll() {
    regionScores.north.morning = { score: 0, weight: 0 }
    regionScores.north.afternoon = { score: 0, weight: 0 }
    regionScores.south.morning = { score: 0, weight: 0 }
    regionScores.south.afternoon = { score: 0, weight: 0 }
    updateSingle({ region: 'north', time: 'morning', waitToPostAll: true })
    updateSingle({ region: 'north', time: 'afternoon', waitToPostAll: true })
    updateSingle({ region: 'south', time: 'morning', waitToPostAll: true })
    updateSingle({ region: 'south', time: 'afternoon', waitToPostAll: true })
}

function updateSingle({ region, time, waitToPostAll }) {
    const request = new XMLHttpRequest()
    regionScores[region][time] = { score: 0, weight: 0 }
    request.onload = function () {
        requestOnLoad.call(this, { region, time })
    }

    // Send one HTTP request at a time per region and time frame
    function loop(i, length, waitToPostAll) {
        if (i >= length) {
            if (waitToPostAll) {
                if (regionScores.north.morning.score > 0
                    && regionScores.north.afternoon.score > 0
                    && regionScores.south.morning.score > 0
                    && regionScores.south.afternoon.score > 0) {
                    const { score: northMorningTotalScore, weight: northMorningTotalWeight } = regionScores.north.morning
                    const northMorningWeightedScore = calculateWeightedScore({ score: northMorningTotalScore, weight: northMorningTotalWeight })

                    const { score: northAfternoonTotalScore, weight: northAfternoonTotalWeight } = regionScores.north.afternoon
                    const northAfternoonWeightedScore = calculateWeightedScore({ score: northAfternoonTotalScore, weight: northAfternoonTotalWeight })

                    const { score: southMorningTotalScore, weight: southMorningTotalWeight } = regionScores.south.morning
                    const southMorningWeightedScore = calculateWeightedScore({ score: southMorningTotalScore, weight: southMorningTotalWeight })

                    const { score: southAfternoonTotalScore, weight: southAfternoonTotalWeight } = regionScores.south.afternoon
                    const southAfternoonWeightedScore = calculateWeightedScore({ score: southAfternoonTotalScore, weight: southAfternoonTotalWeight })

                    const objectToPost = {
                        type: 'new_scores',
                        northMorning: northMorningWeightedScore,
                        northAfternoon: northAfternoonWeightedScore,
                        southMorning: southMorningWeightedScore,
                        southAfternoon: southAfternoonWeightedScore
                    }

                    console.log('[PebbleKit JS]: Posting to Pebble: ' + JSON.stringify(objectToPost))
                    Pebble.sendAppMessage(objectToPost)
                }
            } else {
                // Calculate the weighted score for all points of a region
                const { score: totalScore, weight: totalWeight } = regionScores[region][time]
                const weightedScore = calculateWeightedScore({ score: totalScore, weight: totalWeight })

                const objectToPost = { type: 'new_score', region, time, score: weightedScore }
                console.log('[PebbleKit JS]: Posting to Pebble: ' + JSON.stringify(objectToPost))
                Pebble.sendAppMessage(objectToPost)
            }
            return

        }
        var url = getUrl({ lat: regionCoords[region][i].lat, long: regionCoords[region][i].long, time })

        request.open("GET", url)
        request.onreadystatechange = function () {
            if (this.readyState === XMLHttpRequest.DONE && this.status === 200)
                loop(i + 1, length, waitToPostAll)
        }
        request.send()
    }
    loop(0, regionCoords[region].length, waitToPostAll)
}

Pebble.on('ready', function () {
    console.log('[PebbleKit JS]: PKJS is Ready!')
    Pebble.sendAppMessage({ type: 'ready' })
})

Pebble.addEventListener('appmessage', function (event) {
    console.log('[PebbleKit JS]: Received message: ' + JSON.stringify(event.payload))
    if (event.payload) {
        switch (event.payload.type) {
            case 'ready':
                console.log('[PebbleKit JS]: C is Ready!')
                break
            case 'update_all':
                console.log('[PebbleKit JS]: Got an update_all request!')
                updateAll()
                break
            case 'update_single':
                console.log('[PebbleKit JS]: Got an update_single request!')
                updateSingle({ region: event.payload.region, time: event.payload.time })
                break
        }
    }
})
