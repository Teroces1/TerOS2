import struct

MODE_STRUCT_SIZE = 256

with open("modes.bin", "rb") as f:
    i = 0
    while True:
        data = f.read(MODE_STRUCT_SIZE)
        if not data or len(data) < MODE_STRUCT_SIZE:
            break

        # Extract fields directly using their exact VBE 2.0 structure offsets:
        # Offset 18 (0x12): x_resolution (uint16_t)
        # Offset 20 (0x14): y_resolution (uint16_t)
        # Offset 25 (0x19): bits_per_pixel (uint8_t)
        # Offset 40 (0x28): physical_base_ptr (uint32_t)
        attr = struct.unpack_from("<H", data, offset=0)[0]
        x_res, y_res = struct.unpack_from("<HH", data, offset=18)
        bpp = struct.unpack_from("<B", data, offset=25)[0]
        framebuffer = struct.unpack_from("<I", data, offset=40)[0]

        print(
            f"Mode {i:02d}: {x_res}x{y_res} @ {bpp}bpp | Framebuffer: 0x{framebuffer:08X} | attr: {attr:04X}"
        )
        i += 1