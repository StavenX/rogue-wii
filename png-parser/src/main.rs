use std::fs;

fn main() {
    // args
    let mut args = std::env::args().skip(1);
    let image_path = args.next();
    let header_path = args.next();

    if let (Some(image_path), Some(header_path)) = (image_path, header_path) {
        // image data
        let decoder = png::Decoder::new(fs::File::open(image_path).unwrap());
        let (info, _) = decoder.read_info().unwrap();
        let image_width = format!("#define IMAGE_WIDTH {}", info.width);
        let image_height = format!("#define IMAGE_HEIGHT {}", info.height);
        let header_data = format!("{}\n{}", image_width, image_height);

        // write to header file
        fs::write(header_path, header_data).unwrap_or_else(|e| {
            dbg!(e);
        });
    } else {
        println!("USAGE:");
        println!("    png_parser <image_path> <header_path>");
        println!("INFO:");
        println!("    produces header file info based on image width and height");
    }
}
