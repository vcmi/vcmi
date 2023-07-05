use std::fs::File;
use std::io::BufReader;
use std::io::Read;


#[macro_export] 
macro_rules! try_init_connection {
    ($init:expr) => {
        match unsafe { CONNECTION.get_or_try_init($init) } {
            Ok(connection) => connection,
            Err(e) => {
                println!("Can't create connection: {}", e);
                return 0;
            }
        }
    };
}

pub fn get_file_as_byte_vec(filename: String) -> Vec<u8> {
    println!("Filename: {:?}", filename);
    let f = File::open(&filename).unwrap();
    let mut reader = BufReader::new(f);
    let mut buffer = Vec::new();

    // Read file into vector.
    reader.read_to_end(&mut buffer).unwrap();

    buffer
    // vec![6, 6, 6, 6, 6, 6]
}
