# Adds a colour code from 1 to 4 for the relation table in Land.db so atlas application knows how to colour the polygons that are associated with that relation

import sqlite3
import networkx as nx #This includes a colour graph algorithm 
import sys # Import the sys module

# --- Main Script ---
def main():

    # Check if a database file was provided as a command-line argument
    if len(sys.argv) < 2:
        print("Usage: python your_script_name.py <database_file>")
        sys.exit(1) # Exit with an error code

    DATABASE_FILE = sys.argv[1] # Get the filename from the first command-line argument

    conn = None # Initialize conn to None
    try:
        # Establish a connection to the SQLite database
        conn = sqlite3.connect(DATABASE_FILE)
        cursor = conn.cursor()

        print(f"Successfully connected to database: {DATABASE_FILE}")

        # SQL query to select fromRelationName and toRelationName from tbl_Borders
        # We use DISTINCT to ensure we only get unique border pairs, though NetworkX handles duplicates well.
        query = "SELECT DISTINCT fromRelationID, toRelationID FROM tbl_Borders WHERE fromRelationID IS NOT NULL AND toRelationID IS NOT NULL;"

        cursor.execute(query)

        # Fetch all the results
        borders = cursor.fetchall()

        print(f"Found {len(borders)} border entries in tbl_Borders.")
        # You can uncomment the line below to see the first few borders, for verification
        # if borders:
             # print("First 5 borders:", borders[:5])

        G = nx.Graph() # Create an empty undirected graph

        # Add nodes and edges to the graph
        # Each 'border' is a tuple (fromRelationName, toRelationName)
        for from_id, to_id in borders:
            G.add_edge(from_id, to_id) 

        print(f"Graph created with {G.number_of_nodes()} relations (nodes) and {G.number_of_edges()} borders (edges).")

        # You can print a few nodes and their neighbors for verification (optional)
        # print("\nSample colored relations (ID: Color):")
        # for i, node in enumerate(G.nodes()):
        #     if i >= 5: # Print details for first 5 nodes only
        #         break
        #     print(f"  {node}: Neighbors: {list(G.neighbors(node))}")
        # if G.number_of_nodes() > 5:
        #     print("  ...")
        print("\nAttempting to color the graph...")
        try:
            # nx.coloring.greedy_color attempts to color the graph.
            # 'largest_first' strategy orders nodes by degree (most connections first),
            # which often leads to better colorings for greedy algorithms.
            node_colors_raw = nx.coloring.greedy_color(G, strategy="largest_first")

            # Map the colors from 0-indexed (networkx default) to 1-4
            relation_colors = {node_id: color_idx + 1 for node_id, color_idx in node_colors_raw.items()}

            # Check if more than 4 colors were actually used. For planar graphs this should not happen.
            max_used_color_id = 0
            if relation_colors: # Ensure there are actually colored nodes
                max_used_color_id = max( relation_colors.values())

            if max_used_color_id > 4:
                print(f"Warning: The coloring used {max_used_color_id} colors, which is more than 4."
                      " This might indicate a non-planar graph or an issue with the data/algorithm.")
            else:
                print(f"Graph successfully colored using {max_used_color_id} colors (1 to {max_used_color_id}).")

            # Optional: Print a few colored countries for verification
            print("\nSample colored relations (ID: Color):")
            for i, (relation_id, color) in enumerate(list(relation_colors.items())[:5]):

                print(f"  {relation_id}: Color {color}")

            if len(relation_colors) > 5:
                print("  ...")

            print("\nUpdating tbl_Relations with colourIndex...")
            cursor.execute("""
                SELECT T.relationID
                FROM tbl_Relations AS T
                JOIN spt_Polygons AS P ON T.relationID = P.elementID
                WHERE P.elementType = 3;
            """)
            all_relevant_relation_ids = [row[0] for row in cursor.fetchall()]

            updates_needed = []
            for rel_id in all_relevant_relation_ids:
                color = relation_colors.get(rel_id) # Get color if it exists from graph, else None
                updates_needed.append((color, rel_id))

            update_query = "UPDATE tbl_Relations SET colourIndex = ? WHERE relationID = ?;"
            cursor.executemany(update_query, updates_needed)

            conn.commit() # Commit the changes to the database

            print(f"Updated colourIndex for {len(updates_needed)} relations in tbl_Relations.")

        except nx.NetworkXUnfeasible:
            print("Error: Graph coloring with the given constraints is not feasible.")
            print("This should not occur for planar graphs with 4 colors available.")
            return # Exit if coloring failed
        except Exception as e:
            print(f"An unexpected error occurred during coloring: {e}")
            return # Exit if an error occurs

    except sqlite3.Error as e:
        print(f"Database error: {e}")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")
    finally:
        # Ensure the database connection is closed, even if errors occur
        if conn:
            conn.close()
            print("Database connection closed.")

if __name__ == "__main__":
    main()