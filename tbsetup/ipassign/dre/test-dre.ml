(*
 * EMULAB-COPYRIGHT
 * Copyright (c) 2005-2006 University of Utah and the Flux Group.
 * All rights reserved.
 *)
(*
 *
 * test-dre.ml
 * Test functions for my DRE implementation
 *)

let src = ref None;;
let dst = ref None;;
let edgefile = ref None;;

let argspec =
  [ "-s", Arg.Int (fun s -> src := Some s),
      "Source node";
    "-d", Arg.Int (fun d -> dst := Some d),
      "Destination node"; 
  ];;

let rec compute_all_hops (g : ('a, 'b) Graph.t) =
    let hops = Array.make_matrix (Graph.count_nodes g) (Graph.count_nodes g) (-1) in
    let fill_array (base : unit) (node : (int, 'a) Graph.node) : unit =
        let node_id = node.Graph.node_contents in
        match (Dijkstra.run_dijkstra g node) with (_,pred) ->
        hops.(node_id) <- Dijkstra.get_first_hops g pred node;
        base
    in
    Graph.fold_nodes g fill_array ();
    hops
;;

Arg.parse argspec (fun x -> edgefile := Some x) "";;

let edgestr = match !edgefile with
                None -> raise (Failure "Need an arg")
              | Some(x) -> x in       
let (g,_) = Graph.read_subgraph_file edgestr in 
let hops = compute_all_hops g in
match !src with 
  Some(x) -> begin
      match !dst with 
        Some(y) -> begin
          print_float (Dre.pairwise_dre hops x y None); print_newline();
        end
      | None -> raise (Failure "Dst not given")
  end
| None -> begin
    let dre_table = Dre.compute_dre hops in
    let print_cell (y : int) (x : int) (cell : float) : unit =
        (* if x > 0 then
            if x > y then *)
                Printf.printf "%0.4f\t" cell
                (*
            else
                print_string "\t" *) in
    let print_row (y : int) (row : float array) : unit =
        (* print_int y;
        print_string "\t"; *)
        Array.iteri (print_cell y) row;
        print_newline() in
    let print_header (max : int) : unit =
        print_string "\t";
        for i = 1 to max do
            print_int i;
            print_string "\t";
        done;
        print_newline()
    in
    let print_dre_table (table : float array array) : unit =
        Array.iteri print_row table in
    (* print_header ((Array.length dre_table) - 1); *)
    print_dre_table dre_table
end
