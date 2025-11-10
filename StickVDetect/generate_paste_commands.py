"""
Simple upload - manually paste into REPL
"""

print("="*60)
print("SIMPLE UPLOAD METHOD")
print("="*60)
print("\n1. Open serial terminal:")
print("   python -m serial.tools.miniterm COM12 115200")
print("\n2. Press Ctrl+C to stop running program")
print("   You should see >>> prompt")
print("\n3. Copy and paste each block below:\n")
print("="*60)
print("BLOCK 1: config.py")
print("="*60)

with open('config.py', 'r') as f:
    config_content = f.read()
    
print(f"""
f = open('/flash/config.py', 'w')
f.write('''{config_content}''')
f.close()
print('config.py uploaded')
""")

print("\n" + "="*60)
print("BLOCK 2: boot.py")
print("="*60)

with open('boot.py', 'r') as f:
    boot_content = f.read()
    
print(f"""
f = open('/flash/boot.py', 'w')
f.write('''{boot_content}''')
f.close()
print('boot.py uploaded')
""")

print("\n" + "="*60)
print("BLOCK 3: main.py (LARGE - will be in parts)")
print("="*60)

with open('main.py', 'r') as f:
    main_content = f.read()

# Split into chunks
chunk_size = 2000
chunks = [main_content[i:i+chunk_size] for i in range(0, len(main_content), chunk_size)]

for i, chunk in enumerate(chunks):
    mode = 'w' if i == 0 else 'a'
    print(f"""
f = open('/flash/main.py', '{mode}')
f.write('''{chunk}''')
f.close()
print('main.py part {i+1}/{len(chunks)}')
""")

print("\n" + "="*60)
print("FINALLY: Reset")
print("="*60)
print("""
import machine
machine.reset()
""")

print("\n" + "="*60)
print("Done! Device will reboot and run your tracker!")
print("="*60)
