(*
 * Functions for computing DRE
 *)

(* let compute_dre (graph : ('a, 'b) Graph.t) : float array array = *)
let compute_dre (hops : ('a, 'b) Dijkstra.first_hop array array) : float array array =
    (* Initialize the array of results *)
    let n = Array.length hops in
    let dre_matrix = Array.make_matrix n n 42.0 in
    (* Helper function to decide if two nodes are equivalent from the
     * perspective of another *)
    let are_equivalent (u : int) (v : int) (w : int) : bool =
        Dijkstra.fh_equal hops.(w).(u) hops.(w).(v) in
    (* For now, try the simple, naive, way *)
    for i = 0 to (n - 1) do
        for j = 0 to (n - 1) do
            if i != j then
            let num_equiv = ref 0 in
            for k = 0 to (n - 1) do
                if (k != i && k != j) then
                if are_equivalent i j k then num_equiv := (!num_equiv + 1)
            done;
            dre_matrix.(i).(j) <- ((float_of_int !num_equiv) /. (float_of_int (n - 2)))
        done
    done;
    dre_matrix
;;
