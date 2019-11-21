#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mpi.h>

class Matrix2D {
public:
    int len;
    int row_len;
    int col_len;
    int size;
    double *data;

    Matrix2D(int _row_len, int _col_len)
        : row_len(_row_len), col_len(_col_len), len(_col_len), size(_row_len * _col_len) {
        data = new double[_row_len * _col_len];
    }
    ~Matrix2D() { delete [] data; }
    double& operator()(int row, int col) { return data[row * col_len + col]; }
    Matrix2D& clear() {
        for (int i = 0; i < row_len; i++) {
            for (int j = 0; j < col_len; j++) {
                data[i * col_len + j] = 0;
            }
        }
        return *this;
    }
    void copy(Matrix2D& mat) {
        for (int i = 0; i < row_len; i++) {
            for (int j = 0; j < col_len; j++) {
                data[i * col_len + j] = mat(i, j);
            }
        }
    }
    void copy_l(Matrix2D& mat) {
        for (int i = 0; i < row_len; i++) {
            for (int j = 0; j < i; j++) {
                data[i * col_len + j] = mat(i, j);
            }
            data[i * col_len + i] = 1;
            for (int j = i+1; j < col_len; j++) {
                data[i * col_len + j] = 0;
            }
        }
    }
    void copy_u(Matrix2D& mat) {
        for (int i = 0; i < mat.row_len; i++) {
            for (int j = 0; j < i; j++) {
                data[i * col_len + j] = 0;
            }
            data[i * col_len + i] = mat(i, i);
            for (int j = i+1; j < mat.col_len; j++) {
                data[i * col_len + j] = mat(i, j);
            }
        }
        for (int i = mat.row_len; i < row_len; i++) {
            for (int j = 0; j < col_len; j++) {
                data[i * col_len + j] = 0;
            }
        }
    }
};

void lu_decompose(Matrix2D& mat) {
    for (int k = 0; k < mat.len; k++) {
        for (int i = k + 1; i < mat.len; i++) {
            mat(i, k) /= mat(k, k);
        }
        for (int i = k + 1; i < mat.len; i++) {
            for (int j = k + 1; j < mat.len; j++) {
                mat(i, j) -= mat(i, k) * mat(k, j);
            }
        }
    }
}

void l_inverse(Matrix2D& mat) {
    for (int i = 0; i < mat.len; i++) {
        mat(i, i) = 1;
        for (int j = 0; j < i; j++) {
            double sum = 0;
            for (int k = j; k < i; k++) {
                sum -= mat(i, k) * mat(k, j);
            }
            mat(i, j) = sum;
        }
        for (int j = i+1; j < mat.len; j++) {
            mat(i, j) = 0;
        }
    }
}

void u_inverse(Matrix2D& mat) {
    for (int i = mat.len-1; i >= 0; i--) {
        for (int j = mat.len-1; j > i; j--) {
            double sum = 0;
            for (int k = j; k > i; k--) {
                sum -= mat(i, k) * mat(k, j);
            }
            mat(i, j) = sum / mat(i, i);
        }
        mat(i, i) = 1 / mat(i, i);
        for (int j = i-1; j >= 0; j--) {
            mat(i, j) = 0;
        }
    }
}

void l_multiply(Matrix2D& mat_u, Matrix2D& mat_l, Matrix2D& mat) {
    for (int i = 0; i < mat_u.row_len; i++) {
        for (int j = 0; j < mat_u.col_len; j++) {
            mat_u(i, j) = 0;
            for (int k = 0; k <= i; k++) {
                mat_u(i, j) += mat_l(i, k) * mat(k, j);
            }
        }
    }
}

void u_multiply(Matrix2D& mat_l, Matrix2D& mat, Matrix2D& mat_u) {
    for (int i = 0; i < mat_l.row_len; i++) {
        for (int j = 0; j < mat_l.col_len; j++) {
            mat_l(i, j) = 0;
            for (int k = 0; k <= j; k++) {
                mat_l(i, j) += mat(i, k) * mat_u(k, j);
            }
        }
    }
}

void lu_multiply(Matrix2D& mat_out, Matrix2D& mat1, Matrix2D& mat2, int mode = 0) {
    for (int i = 0; i < mat_out.row_len; i++) {
        for (int j = 0; j < mat_out.col_len; j++) {
            double sum = 0;
            for (int k = 0; k < mat1.col_len; k++) {
                sum += mat1(i, k) * mat2(k, j);
            }
            if (mode == -1) {
                mat_out(i, j) -= sum;
            } else if (mode == +1) {
                mat_out(i, j) += sum;
            } else {
                mat_out(i, j) = sum;
            }
        }
    }
}

void lu_extract(Matrix2D& mat_l, Matrix2D& mat_u, Matrix2D& mat_lu) {
    for (int i = 0; i < mat_lu.row_len; i++) {
        for (int j = 0; j < i; j++) {
            mat_l(i, j) = mat_lu(i, j);
            mat_u(i, j) = 0;
        }
        mat_l(i, i) = 1;
        mat_u(i, i) = mat_lu(i, i);
        for (int j = i+1; j < mat_lu.col_len; j++) {
            mat_l(i, j) = 0;
            mat_u(i, j) = mat_lu(i, j);
        }
        for (int j = mat_lu.col_len; j < mat_l.col_len; j++) {
            mat_l(i, j) = 0;
        }
    }
    for (int i = mat_lu.row_len; i < mat_u.row_len; i++) {
        for (int j = 0; j < mat_u.col_len; j++) {
            mat_u(i, j) = 0;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <matrix_size> <seed_number>\n", argv[0]);
        return -1;
    }

    int mat_len = atoi(argv[1]);
    int seed = atoi(argv[2]);

    if (MPI_Init(NULL, NULL) != 0) {
        printf("Error: MPI_Init failed\n");
        return -1;
    }

    // MPI Region
    {
        // Initialize MPI Communicator
        int rank, comm_size;
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        MPI_Comm_size(MPI_COMM_WORLD, &comm_size);

        int comm_per_line = (int)sqrt(comm_size);
        MPI_Comm *row_comm = new MPI_Comm[comm_per_line-1];
        MPI_Comm *col_comm = new MPI_Comm[comm_per_line-1];
        int row_order = rank / comm_per_line;
        int col_order = rank % comm_per_line;
        int calc_order = (row_order < col_order ? row_order : col_order);

        MPI_Comm_split(MPI_COMM_WORLD, rank / comm_per_line, rank, &row_comm[0]);
        MPI_Comm_split(MPI_COMM_WORLD, rank % comm_per_line, rank, &col_comm[0]);
        for (int i = 1; i <= calc_order+1 && i < comm_per_line-1; i++) {
            MPI_Comm_split(row_comm[i-1], col_order >= i, rank, &row_comm[i]);
            MPI_Comm_split(col_comm[i-1], row_order >= i, rank, &col_comm[i]);
        }

        // Allocation
        int sub_mat_len = mat_len / comm_per_line + (mat_len % comm_per_line == 0 ? 0 : 1);
        int sub_mat_row_len = sub_mat_len - (row_order == comm_per_line-1 ? comm_per_line - mat_len % comm_per_line : 0);
        int sub_mat_col_len = sub_mat_len - (col_order == comm_per_line-1 ? comm_per_line - mat_len % comm_per_line : 0);

        Matrix2D sub_mat(sub_mat_row_len, sub_mat_col_len);
        Matrix2D sub_mat_lu(sub_mat_row_len, sub_mat_col_len);
        Matrix2D sub_mat_l(sub_mat_row_len, sub_mat_len);
        Matrix2D sub_mat_u(sub_mat_len, sub_mat_col_len);

        // Build Matrix
        srand(seed);
        for (int row = 0; row < sub_mat_len * row_order; row++) {
            for (int col = 0; col < mat_len; col++) {
                rand();
            }
        }
        for (int row = 0; row < sub_mat_row_len; row++) {
            for (int col = 0; col < sub_mat_len * col_order; col++) {
                rand();
            }
            for (int col = 0; col < sub_mat_col_len; col++) {
                sub_mat(row, col) = sub_mat_lu(row, col) = (int)rand() / 1000;
            }
            for (int col = sub_mat_len * col_order + sub_mat_col_len; col < mat_len; col++) {
                rand();
            }
        }

        // LU Decomposition
        for (int step = 0; step <= calc_order; step++) {
            if (row_order == step && col_order == step) {
                lu_decompose(sub_mat_lu);
                if (step < comm_per_line-1) {
                    MPI_Bcast(sub_mat_lu.data, sub_mat_lu.size, MPI_DOUBLE, 0, row_comm[step]); // Send L
                    MPI_Bcast(sub_mat_lu.data, sub_mat_lu.size, MPI_DOUBLE, 0, col_comm[step]); // Send U
                }
                sub_mat_l.copy_l(sub_mat_lu);
                sub_mat_u.copy_u(sub_mat_lu);
            } else if (row_order == step) {
                MPI_Bcast(sub_mat_l.data, sub_mat_l.size, MPI_DOUBLE, 0, row_comm[step]); // Receive L
                l_inverse(sub_mat_l);
                l_multiply(sub_mat_lu, sub_mat_l, sub_mat);
                MPI_Bcast(sub_mat_lu.data, sub_mat_lu.size, MPI_DOUBLE, 0, col_comm[step]); // Send U
                sub_mat_u.copy(sub_mat_lu);
            } else if (col_order == step) {
                MPI_Bcast(sub_mat_u.data, sub_mat_u.size, MPI_DOUBLE, 0, col_comm[step]); // Receive U
                u_inverse(sub_mat_u);
                u_multiply(sub_mat_lu, sub_mat, sub_mat_u);
                MPI_Bcast(sub_mat_lu.data, sub_mat_lu.size, MPI_DOUBLE, 0, row_comm[step]); // Send L
                sub_mat_l.copy(sub_mat_lu);
            } else {
                MPI_Bcast(sub_mat_l.data, sub_mat_l.size, MPI_DOUBLE, 0, row_comm[step]); // Receive L
                MPI_Bcast(sub_mat_u.data, sub_mat_u.size, MPI_DOUBLE, 0, col_comm[step]); // Receive U
                lu_multiply(sub_mat_lu, sub_mat_l, sub_mat_u, -1);
            }
        }

        // Reconstruct Matrix
        Matrix2D& sub_mat_recon = sub_mat_lu.clear();
        Matrix2D sub_mat_l_buf(sub_mat_row_len, sub_mat_len);
        Matrix2D sub_mat_u_buf(sub_mat_len, sub_mat_col_len);
        Matrix2D *sub_mat_l_cur = &sub_mat_l;
        Matrix2D *sub_mat_l_next = &sub_mat_l_buf;
        Matrix2D *sub_mat_u_cur = &sub_mat_u;
        Matrix2D *sub_mat_u_next = &sub_mat_u_buf;
        MPI_Request req[4];
        bool req_valid[4] = {false, false, false, false};
        if (row_order >= col_order) {
            if (row_order < comm_per_line-1) {
                MPI_Isend(sub_mat_l.data, sub_mat_l.size, MPI_DOUBLE, comm_per_line - row_order + col_order - 1, 0, row_comm[0], &req[0]);  // Send L
                req_valid[0] = true;
            } else {
                sub_mat_l_cur = &sub_mat_l_buf;
                sub_mat_l_next = &sub_mat_l;
            }
        }
        if (row_order <= col_order) {
            if (col_order < comm_per_line-1) {
                MPI_Isend(sub_mat_u.data, sub_mat_u.size, MPI_DOUBLE, comm_per_line - col_order + row_order - 1, 0, col_comm[0], &req[1]); // Send U
                req_valid[1] = true;
            } else {
                sub_mat_u_cur = &sub_mat_u_buf;
                sub_mat_u_next = &sub_mat_u;
            }
        }
        if (col_order >= comm_per_line - row_order - 1) {
            if (row_order < comm_per_line-1) {
                MPI_Irecv(sub_mat_l_buf.data, sub_mat_l_buf.size, MPI_DOUBLE, - comm_per_line + row_order + col_order + 1, 0, row_comm[0], &req[2]); // Receive L
                req_valid[2] = true;
            }
            if (col_order < comm_per_line-1) {
                MPI_Irecv(sub_mat_u_buf.data, sub_mat_u_buf.size, MPI_DOUBLE, - comm_per_line + row_order + col_order + 1, 0, col_comm[0], &req[3]); // Receive U
                req_valid[3] = true;
            }
        }
        for (int i = 0; i < 4; i++) {
            if (req_valid[i] == true) {
                MPI_Wait(&req[i], MPI_STATUS_IGNORE);
                req_valid[i] = false;
            }
        }

        int l_dest = (col_order + 1) % comm_per_line;
        int l_src = (col_order - 1 < 0 ? comm_per_line - 1 : col_order - 1);
        int u_dest = (row_order + 1) % comm_per_line;
        int u_src = (row_order - 1 < 0 ? comm_per_line - 1 : row_order - 1);
        for (int step = 0; step < comm_per_line; step++) {
            bool row_activate = (step + (step < col_order + 1 ? comm_per_line : 0) <= row_order + col_order + 1);
            bool col_activate = (step + (step < row_order + 1 ? comm_per_line : 0) <= row_order + col_order + 1);
            bool next_row_activate = (step+1 + (step+1 < col_order + 1 ? comm_per_line : 0) <= row_order + col_order + 1);
            bool next_col_activate = (step+1 + (step+1 < row_order + 1 ? comm_per_line : 0) <= row_order + col_order + 1);
            bool calc_activate = row_activate && col_activate;

            Matrix2D* temp_l = sub_mat_l_next; sub_mat_l_next = sub_mat_l_cur; sub_mat_l_cur = temp_l;
            Matrix2D* temp_u = sub_mat_u_next; sub_mat_u_next = sub_mat_u_cur; sub_mat_u_cur = temp_u;

            if (row_activate == true && step < comm_per_line-1) {
                MPI_Isend(sub_mat_l_cur->data, sub_mat_l_cur->size, MPI_DOUBLE, l_dest, step+1, row_comm[0], &req[0]);
                req_valid[0] = true;
            }
            if (col_activate == true && step < comm_per_line-1) {
                MPI_Isend(sub_mat_u_cur->data, sub_mat_u_cur->size, MPI_DOUBLE, u_dest, step+1, col_comm[0], &req[1]);
                req_valid[1] = true;
            }
            if (next_row_activate == true && step < comm_per_line-1) {
                MPI_Irecv(sub_mat_l_next->data, sub_mat_l_next->size, MPI_DOUBLE, l_src, step+1, row_comm[0], &req[2]);
                req_valid[2] = true;
            }
            if (next_col_activate == true && step < comm_per_line-1) {
                MPI_Irecv(sub_mat_u_next->data, sub_mat_u_next->size, MPI_DOUBLE, u_src, step+1, col_comm[0], &req[3]);
                req_valid[3] = true;
            }
            if (calc_activate == true) {
                lu_multiply(sub_mat_recon, *sub_mat_l_cur, *sub_mat_u_cur, +1);
            }
            for (int i = 0; i < 4; i++) {
                if (req_valid[i] == true) {
                    MPI_Wait(&req[i], MPI_STATUS_IGNORE);
                    req_valid[i] = false;
                }
            }
        }

        // Calculate Matrix Difference
        double *col_diff = new double[sub_mat_recon.col_len]();
        for (int j = 0; j < sub_mat_recon.col_len; j++) {
            for (int i = 0; i < sub_mat_recon.row_len; i++) {
                col_diff[j] += (sub_mat(i, j) - sub_mat_recon(i, j)) * (sub_mat(i, j) - sub_mat_recon(i, j));
            }
        }

        double *col_diff_sum = (row_order == 0 ? new double[sub_mat_recon.col_len] : NULL);
        MPI_Reduce(col_diff, col_diff_sum, sub_mat_recon.col_len, MPI_DOUBLE, MPI_SUM, 0, col_comm[0]);

        if (row_order == 0) {
            double local_diff = 0;
            for (int i = 0; i < sub_mat_recon.col_len; i++) {
                local_diff += sqrt(col_diff_sum[i]);
            }

            double global_diff = 0;
            MPI_Reduce(&local_diff, &global_diff, 1, MPI_DOUBLE, MPI_SUM, 0, row_comm[0]);

            if (col_order == 0) {
                printf("%lf\n", global_diff);
            }
        }
    }

    if (MPI_Finalize() != 0) {
        printf("Error: MPI_Finalize failed\n");
        return -1;
    }

    return 0;
}
