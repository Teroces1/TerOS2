import requests
from bs4 import BeautifulSoup
import re
import time

def fetch_pokemon_data(pokemon_name):
    # Format the name for the URL (lowercase, replace spaces with hyphens)
    formatted_name = pokemon_name.lower().replace(" ", "-").replace("'", "")
    url = f"https://pokemondb.net/pokedex/{formatted_name}"
    
    # Add a User-Agent to prevent getting blocked by the website
    headers = {
        "User-Agent": "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36"
    }
    
    response = requests.get(url, headers=headers)
    
    if response.status_code != 200:
        print(f"Error: Could not find Pokémon '{pokemon_name}' (Status code: {response.status_code})")
        return None

    soup = BeautifulSoup(response.content, 'html.parser')
    
    # Helper function to find a table header (th) and get its corresponding value (td)
    def get_table_value(header_text):
        th = soup.find('th', string=re.compile(f"^{header_text}$", re.IGNORECASE))
        if th:
            td = th.find_next_sibling('td')
            if td:
                # Clean up whitespace and non-breaking spaces
                return re.sub(r'\s+', ' ', td.text.replace('\xa0', ' ')).strip()
        return "N/A"

    # 1. Fetch Types
    types = []
    type_th = soup.find('th', string='Type')
    if type_th:
        type_td = type_th.find_next_sibling('td')
        if type_td:
            types = [a.text for a in type_td.find_all('a', class_='type-icon')]

    # 2. Fetch Size (Height), Weight, and Rarity (Catch rate)
    height = get_table_value('Height')
    weight = get_table_value('Weight')
    rarity = get_table_value('Catch rate')

    # 3. Fetch Languages
    # We target the "Other languages" section specifically
    languages_to_fetch = [
        'English', 'Japanese', 'Korean', 
        'Chinese (Simplified)', 'Chinese (Traditional)', 
        'French', 'German'
    ]
    
    languages = {}
    for lang in languages_to_fetch:
        languages[lang] = get_table_value(lang)

    # Compile the final data dictionary
    pokemon_data = {
        "Name": pokemon_name.capitalize(),
        "Types": types,
        "Size (Height)": height,
        "Weight": weight,
        "Rarity (Catch Rate)": rarity,
        "Languages": languages
    }
    
    return pokemon_data

pokemons = ["pikachu", "gyarados", "charizard"]

# --- Example Usage ---
if __name__ == "__main__":
    # You can change this to any Pokemon name
    for v in pokemons:
        target_pokemon = v 
        
        print(f"Fetching data for {target_pokemon}...\n")
        data = fetch_pokemon_data(target_pokemon)
        
        if data:
            print(f"--- {data['Name']} ---")
            print(f"Type(s): {', '.join(data['Types'])}")
            print(f"Size:    {data['Size (Height)']}")
            print(f"Weight:  {data['Weight']}")
            print(f"Rarity:  {data['Rarity (Catch Rate)']}")
            
            print("\n--- Languages ---")
            for lang, translation in data['Languages'].items():
                print(f"{lang.ljust(22)}: {translation}")
        
        time.sleep(3)