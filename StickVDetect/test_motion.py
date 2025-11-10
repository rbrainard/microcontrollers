"""
M5StickV Human Tracker - Object tracking with human detection heuristics
Detects motion, filters for human-sized objects, sends position via UART
"""

import sensor, image, lcd, time
from machine import UART

# Init
lcd.init()
lcd.rotation(2)
sensor.reset()
sensor.set_pixformat(sensor.RGB565)
sensor.set_framesize(sensor.QVGA)  # 320x240
sensor.skip_frames(30)

# UART for sending tracking data (TX on GPIO 4, RX on GPIO 5)
uart = UART(UART.UART1, 115200, 8, None, 1, timeout=1000, read_buf_len=4096)

print("Human Tracker Started")
print("Position data sent via UART")

# Sample 25 points across the frame (5x5 grid)
sample_points = []
for row in range(5):
    for col in range(5):
        x = 40 + col * 60  # 40, 100, 160, 220, 280
        y = 30 + row * 45  # 30, 75, 120, 165, 210
        sample_points.append((x, y))

# Store previous pixel values
prev_pixels = [0] * 25
frame_count = 0
motion_threshold = 25

def cluster_motion_points(motion_points):
    """Group nearby motion points into objects"""
    if not motion_points:
        return []

    clusters = []
    used = [False] * len(motion_points)

    for i, (x1, y1) in enumerate(motion_points):
        if used[i]:
            continue

        # Start new cluster
        cluster = [(x1, y1)]
        used[i] = True

        # Find nearby points (within 80 pixels)
        for j, (x2, y2) in enumerate(motion_points):
            if not used[j]:
                dist = ((x2 - x1) ** 2 + (y2 - y1) ** 2) ** 0.5
                if dist < 80:
                    cluster.append((x2, y2))
                    used[j] = True

        clusters.append(cluster)

    return clusters

def is_human_like(min_x, max_x, min_y, max_y, num_points):
    """Check if object has human-like characteristics"""
    width = max_x - min_x
    height = max_y - min_y

    # Too small (noise or distant object)
    if width < 40 or height < 40:
        return False

    # Too large (whole scene movement)
    if width > 200 or height > 180:
        return False

    # Human-like aspect ratio (height should be >= width, roughly vertical)
    aspect_ratio = height / width if width > 0 else 0
    if aspect_ratio < 0.8 or aspect_ratio > 3.0:
        return False

    # Should have sufficient motion points (not just a corner)
    if num_points < 3:
        return False

    # Position check - humans are usually in middle/lower part of frame
    center_y = (min_y + max_y) // 2
    if center_y < 40:  # Too high (probably not a person)
        return False

    return True

def pixel_to_angle(pixel_x):
    """Convert pixel X to motor angle (-90 to +90)"""
    # Center is 160, map to 0 degrees
    # 0 pixels = -90°, 320 pixels = +90°
    angle = ((pixel_x - 160) / 160) * 90
    return max(-90, min(90, int(angle)))

while True:
    img = sensor.snapshot()
    frame_count += 1

    # Update reference every 20 frames
    if frame_count > 20:
        frame_count = 0
        for i, (x, y) in enumerate(sample_points):
            pixel = img.get_pixel(x, y)
            prev_pixels[i] = pixel[0]
        img.draw_string(5, 5, "REF", color=(255, 255, 0))
    else:
        # Check for motion
        motion_points = []

        for i, (x, y) in enumerate(sample_points):
            if prev_pixels[i] == 0:
                continue

            pixel = img.get_pixel(x, y)
            diff = abs(pixel[0] - prev_pixels[i])

            if diff > motion_threshold:
                motion_points.append((x, y))

        # Cluster motion points into objects
        clusters = cluster_motion_points(motion_points)

        # Filter for human-like objects
        human_candidates = []
        for cluster in clusters:
            if len(cluster) > 0:
                xs = [x for x, y in cluster]
                ys = [y for x, y in cluster]
                min_x, max_x = min(xs), max(xs)
                min_y, max_y = min(ys), max(ys)

                if is_human_like(min_x, max_x, min_y, max_y, len(cluster)):
                    human_candidates.append((cluster, min_x, max_x, min_y, max_y))

        if human_candidates:
            # Track largest human-like object
            largest = max(human_candidates, key=lambda c: len(c[0]))
            cluster, min_x, max_x, min_y, max_y = largest

            # Expand box slightly
            min_x = max(0, min_x - 20)
            max_x = min(320, max_x + 20)
            min_y = max(0, min_y - 20)
            max_y = min(240, max_y + 20)

            center_x = (min_x + max_x) // 2
            center_y = (min_y + max_y) // 2

            # Calculate motor angle
            angle = pixel_to_angle(center_x)

            # Send via UART: "TRACK,<angle>,<x>,<y>\n"
            uart.write("TRACK,{},{},{}\n".format(angle, center_x, center_y))

            # Draw tracked human
            img.draw_rectangle(min_x, min_y, max_x - min_x, max_y - min_y,
                             color=(0, 255, 0), thickness=2)
            img.draw_circle(center_x, center_y, 5, color=(255, 0, 0), thickness=2)

            # Draw motion points
            for x, y in cluster:
                img.draw_circle(x, y, 2, color=(255, 255, 0))

            img.draw_string(5, 5, "HUMAN", color=(0, 255, 0))
            img.draw_string(5, 20, "Ang:{}".format(angle), color=(255, 255, 255))
            print("Human at X={} Angle={}° ({} pts)".format(center_x, angle, len(cluster)))
        else:
            # No human detected
            uart.write("SEARCH\n")
            img.draw_string(5, 5, "Searching", color=(255, 255, 255))

    # Draw crosshair at center
    img.draw_line(150, 120, 170, 120, color=(255, 255, 0))
    img.draw_line(160, 110, 160, 130, color=(255, 255, 0))

    lcd.display(img)
    time.sleep_ms(30)
