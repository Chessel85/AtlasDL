# Adds a colour code from 1 to 4 for the countries table in Land.db so atlas application knows how to colour the polygons that are associated with that country

import sqlite3
import networkx as nx
import sys
import random

# --- Main Script ---
def main():
    # Check if a database file was provided as a command-line argument
    if len(sys.argv) < 2:
        print("Usage: python your_script_name.py <database_file>")
        sys.exit(1)

    DATABASE_FILE = sys.argv[1]

    conn = None
    try:
        # Establish a connection to the SQLite database
        conn = sqlite3.connect(DATABASE_FILE)
        cursor = conn.cursor()

        print(f"Successfully connected to database: {DATABASE_FILE}")

        # SQL query to get all country relations that have a polygon
        # This is a crucial change to ensure all countries (even islands) are considered.
        cursor.execute("SELECT relationID FROM tbl_countries;")
        all_relation_ids = [row[0] for row in cursor.fetchall()]
        
        # Check if we have any countries to color
        if not all_relation_ids:
            print("No country relations found in tbl_countries. Exiting.")
            sys.exit(0)
            
        # SQL query to select fromRelationID and toRelationID from tbl_Borders
        query = "SELECT DISTINCT fromRelationID, toRelationID FROM tbl_Borders;"
        cursor.execute(query)
        borders = cursor.fetchall()

        print(f"Found {len(borders)} border entries in tbl_Borders.")
        
        G = nx.Graph()
        G.add_nodes_from(all_relation_ids) # Add all relations as nodes
        
        # Add edges for all bordering relations
        for from_id, to_id in borders:
            G.add_edge(from_id, to_id)

        print(f"Graph created with {G.number_of_nodes()} relations (nodes) and {G.number_of_edges()} borders (edges).")
        print("\nAttempting to color the graph...")

        try:
            node_colors_raw = nx.coloring.greedy_color(G, strategy="largest_first")
            
            # Map the colors from 0-indexed (networkx default) to 1-4
            relation_colors = {node_id: color_idx + 1 for node_id, color_idx in node_colors_raw.items()}

            # Check for the highest color index used
            max_used_color_id = 0
            if relation_colors:
                max_used_color_id = max(relation_colors.values())

            if max_used_color_id > 4:
                print(f"Warning: The coloring used {max_used_color_id} colors, which is more than 4."
                      " This might indicate a non-planar graph or an issue with the data/algorithm.")
            else:
                print(f"Graph successfully colored using {max_used_color_id} colors (1 to {max_used_color_id}).")

            # Update all relations in tbl_countries
            updates_needed = []
            
            # Use a pre-defined list of colors for random assignment
            colors_for_random = [1, 2, 3, 4]
            
            for rel_id in all_relation_ids:
                color = relation_colors.get(rel_id)
                if color is None:
                    # If a relation is not in the graph (i.e., has no borders), assign a random color
                    color = random.choice(colors_for_random)
                    print(f"Note: Relation {rel_id} has no borders. Assigned random color {color}.")
                updates_needed.append((color, rel_id))

            update_query = "UPDATE tbl_countries SET colourIndex = ? WHERE relationID = ?;"
            cursor.executemany(update_query, updates_needed)
            conn.commit()

            print(f"Updated colourIndex for {len(updates_needed)} relations in tbl_countries.")

        except nx.NetworkXUnfeasible as e:
            print(f"Error: Graph coloring with the given constraints is not feasible. {e}")
            return
        except Exception as e:
            print(f"An unexpected error occurred during coloring: {e}")
            return

    except sqlite3.Error as e:
        print(f"Database error: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
    finally:
        if conn:
            conn.close()
            print("Database connection closed.")

if __name__ == "__main__":
    main()