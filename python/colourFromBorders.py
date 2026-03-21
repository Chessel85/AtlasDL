import sqlite3
import networkx as nx
import sys
import random

def main():
    # Check for correct command-line arguments
    if len(sys.argv) < 3:
        print("Usage: python update_colors.py <database_file> <layer_id>")
        sys.exit(1)

    DATABASE_FILE = sys.argv[1]
    LAYER_ID = sys.argv[2]

    conn = None
    try:
        conn = sqlite3.connect(DATABASE_FILE)
        cursor = conn.cursor()

        print(f"Connected to: {DATABASE_FILE} | Processing Layer: {LAYER_ID}")

        # 1. Fetch all polygon IDs for the specific layer
        # This ensures islands (polygons with no borders) are included
        cursor.execute("SELECT polygonId FROM spt_polygons WHERE layerId = ?;", (LAYER_ID,))
        all_poly_ids = [row[0] for row in cursor.fetchall()]
        
        if not all_poly_ids:
            print(f"No polygons found for layerId {LAYER_ID}. Exiting.")
            sys.exit(0)
            
        # 2. Fetch border relationships for this layer
        # We join with spt_polygons to ensure we only get borders relevant to the current layer
        query = """
            SELECT DISTINCT b.polygon1Id, b.polygon2Id 
            FROM tbl_borders b
            JOIN spt_polygons p ON b.polygon1Id = p.polygonId
            WHERE p.layerId = ?;
        """
        cursor.execute(query, (LAYER_ID,))
        borders = cursor.fetchall()

        print(f"Found {len(borders)} border entries for this layer.")
        
        # 3. Build the Graph
        G = nx.Graph()
        G.add_nodes_from(all_poly_ids)
        G.add_edges_from(borders)

        print(f"Graph created: {G.number_of_nodes()} nodes, {G.number_of_edges()} edges.")

        # 4. Perform Coloring
        # Using largest_first strategy to minimize color count
        try:
            node_colors_raw = nx.coloring.greedy_color(G, strategy="largest_first")
            
            # Map 0-indexed colors to 1-9 range as requested
            # NetworkX starts at 0, so we add 1.
            final_assignments = []
            
            # Use 1-9 for random assignments for isolated polygons
            valid_range = list(range(1, 10))

            for poly_id in all_poly_ids:
                color_idx = node_colors_raw.get(poly_id)
                if color_idx is not None:
                    color = color_idx + 1
                else:
                    # Fallback for nodes with no edges
                    color = random.choice(valid_range)
                
                final_assignments.append((color, poly_id))

            max_used = max([c for c, i in final_assignments])
            print(f"Coloring complete. Max color index used: {max_used}")

            if max_used > 9:
                print(f"Warning: Used {max_used} colors, which exceeds the 1-9 target.")

            # 5. Update the Database
            update_query = "UPDATE spt_polygons SET mapColour = ? WHERE polygonId = ?;"
            cursor.executemany(update_query, final_assignments)
            conn.commit()

            print(f"Successfully updated mapColour for {len(final_assignments)} polygons.")

        except Exception as e:
            print(f"Error during graph processing: {e}")

    except sqlite3.Error as e:
        print(f"Database error: {e}")
    finally:
        if conn:
            conn.close()
            print("Database connection closed.")

if __name__ == "__main__":
    main()